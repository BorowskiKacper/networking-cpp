#include "shared/run_result.h"

#include "shared/benchmark.h"
#include "shared/json_writer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// Stamped in by the Makefile. The defaults keep an ad-hoc compile working; they also make it obvious in the document that a run came from a build that did not record its origin.
#ifndef BENCH_GIT_SHA
#define BENCH_GIT_SHA "unknown"
#endif
#ifndef BENCH_GIT_DIRTY
#define BENCH_GIT_DIRTY 0
#endif
#ifndef BENCH_CXXFLAGS
#define BENCH_CXXFLAGS "unknown"
#endif
#ifndef BENCH_BUILD_CONFIG
#define BENCH_BUILD_CONFIG "unknown"
#endif

namespace bench
{
    namespace
    {
        constexpr int kSchemaVersion = 1;

        std::string Trim(std::string text)
        {
            const size_t first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                return {};
            const size_t last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1);
        }

        std::string ReadTextFile(const std::string &path)
        {
            std::ifstream file(path);
            if (!file.is_open())
                return {};
            std::ostringstream contents;
            contents << file.rdbuf();
            return Trim(contents.str());
        }

        // /proc/cpuinfo style: the value of the first "key : value" line matching key.
        std::string ProcField(const std::string &path, const std::string &key)
        {
            std::ifstream file(path);
            std::string line;
            while (std::getline(file, line))
            {
                if (line.rfind(key, 0) != 0)
                    continue;
                const size_t colon = line.find(':');
                if (colon == std::string::npos)
                    continue;
                return Trim(line.substr(colon + 1));
            }
            return {};
        }

        // "always [madvise] never" -> "madvise", which is how the THP knobs report themselves.
        std::string BracketedChoice(const std::string &text)
        {
            const size_t open = text.find('[');
            const size_t close = text.find(']', open == std::string::npos ? 0 : open);
            if (open == std::string::npos || close == std::string::npos)
                return text;
            return text.substr(open + 1, close - open - 1);
        }

        std::string HexU64(uint64_t value)
        {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%016" PRIx64, value);
            return buffer;
        }

        std::string FormatTime(const std::time_t seconds, const char *format)
        {
            std::tm parts{};
            gmtime_r(&seconds, &parts);
            char buffer[64];
            std::strftime(buffer, sizeof(buffer), format, &parts);
            return buffer;
        }

        // Length plus the head and tail of the file. Enough to notice that the input changed, without a full pass over gigabytes that would also warm the page cache the benchmark is trying to measure against.
        uint64_t FingerprintFile(const std::string &path, uint64_t size)
        {
            constexpr size_t kWindow = 64 * 1024;

            uint64_t hash = Fnv1a(&size, sizeof(size));
            const int fd = ::open(path.c_str(), O_RDONLY);
            if (fd < 0)
                return hash;

            std::vector<char> buffer(kWindow);
            const size_t head = size < kWindow ? static_cast<size_t>(size) : kWindow;
            ssize_t read_bytes = ::pread(fd, buffer.data(), head, 0);
            if (read_bytes > 0)
                hash = Fnv1a(buffer.data(), static_cast<size_t>(read_bytes), hash);

            if (size > kWindow)
            {
                read_bytes = ::pread(fd, buffer.data(), kWindow, static_cast<off_t>(size - kWindow));
                if (read_bytes > 0)
                    hash = Fnv1a(buffer.data(), static_cast<size_t>(read_bytes), hash);
            }

            ::close(fd);
            return hash;
        }

        bool EnsureParentDirectory(const std::string &path, std::string *error)
        {
            const std::filesystem::path parent = std::filesystem::path(path).parent_path();
            if (parent.empty())
                return true;

            std::error_code code;
            std::filesystem::create_directories(parent, code);
            if (code && !std::filesystem::is_directory(parent))
            {
                *error = "could not create " + parent.string() + ": " + code.message();
                return false;
            }
            return true;
        }
    }

    RunResult::RunResult(std::string benchmark, const Config &config)
        : benchmark_(std::move(benchmark)), config_(config), started_(std::chrono::steady_clock::now())
    {
        const auto now = std::chrono::system_clock::now();
        started_at_unix_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
        started_at_ = FormatTime(seconds, "%Y-%m-%dT%H:%M:%SZ");

        // Sortable, unique in practice, and readable: when it ran, what was built, and which configuration -- so two runs of the same configuration are recognisable side by side in a bucket listing.
        run_id_ = FormatTime(seconds, "%Y%m%dT%H%M%SZ") + "-" + BENCH_GIT_SHA + "-" +
                  HexU64(config_.Hash()).substr(0, 8);

        char hostname[256] = {};
        if (gethostname(hostname, sizeof(hostname) - 1) != 0)
            std::strcpy(hostname, "unknown");
        host_.AddString("hostname", hostname);

        // GCE does not put the machine type anywhere on the filesystem, and asking the metadata server would mean an HTTP client in a benchmark binary; the job that launches the run knows it, so it passes it down.
        const char *machine_type = std::getenv("BENCH_MACHINE_TYPE");
        host_.AddString("machine_type", machine_type != nullptr
                                            ? std::string(machine_type)
                                            : ReadTextFile("/sys/class/dmi/id/product_name"));

        host_.AddString("cpu_model", ProcField("/proc/cpuinfo", "model name"));
        host_.AddInt("logical_cpus", sysconf(_SC_NPROCESSORS_ONLN));

        utsname kernel{};
        if (uname(&kernel) == 0)
        {
            host_.AddString("kernel_release", kernel.release);
            host_.AddString("kernel_version", kernel.version);
        }

        host_.AddString("thp_enabled", BracketedChoice(ReadTextFile("/sys/kernel/mm/transparent_hugepage/enabled")));
        host_.AddString("thp_defrag", BracketedChoice(ReadTextFile("/sys/kernel/mm/transparent_hugepage/defrag")));
        host_.AddString("smt", ReadTextFile("/sys/devices/system/cpu/smt/control"));
    }

    void RunResult::Fail(std::string message)
    {
        status_ = "error";
        error_ = std::move(message);
    }

    void RunResult::SetNsPerCycle(double start, double end)
    {
        ns_per_cycle_ = start;
        ns_per_cycle_end_ = end;
    }

    void RunResult::AddPinnedCpu(int cpu_id)
    {
        for (int existing : pinned_cpus_)
        {
            if (existing == cpu_id)
                return;
        }
        pinned_cpus_.push_back(cpu_id);
    }

    void RunResult::SetInputFile(const std::string &path)
    {
        has_input_ = true;
        input_.AddString("path", path);

        struct stat info{};
        if (stat(path.c_str(), &info) != 0)
        {
            input_.AddString("stat_error", std::strerror(errno));
            return;
        }

        const uint64_t size = static_cast<uint64_t>(info.st_size);
        input_.AddUInt("bytes", size);
        input_.AddString("modified_at", FormatTime(info.st_mtime, "%Y-%m-%dT%H:%M:%SZ"));
        input_.AddString("fingerprint", HexU64(FingerprintFile(path, size)));
    }

    RunResult::NamedHistogram RunResult::Capture(std::string name, const HDRHistogram &histogram) const
    {
        NamedHistogram captured;
        captured.name = std::move(name);
        captured.buckets.assign(histogram.Buckets(), histogram.Buckets() + HDRHistogram::BUCKET_COUNT);
        captured.samples = histogram.Samples();
        captured.total_cycles = histogram.TotalCycles();
        captured.clipped = histogram.Clipped();
        captured.max_cycles = histogram.MaxCycles();
        if (captured.samples > 0)
        {
            captured.p50 = histogram.PercentileCycles(0.5);
            captured.p90 = histogram.PercentileCycles(0.9);
            captured.p99 = histogram.PercentileCycles(0.99);
            captured.p99_9 = histogram.PercentileCycles(0.999);
            captured.p99_99 = histogram.PercentileCycles(0.9999);
        }
        return captured;
    }

    void RunResult::AddHistogram(std::string name, const HDRHistogram &histogram)
    {
        histograms_.push_back(Capture(std::move(name), histogram));
    }

    void RunResult::AddMessageTypeHistograms(const HDRHistogram *by_type)
    {
        for (size_t type = 0; type < 256; type++)
        {
            if (by_type[type].Empty())
                continue;
            // Printable ITCH types are named by their letter; anything else would be a parser bug, and is recorded by its byte value so it cannot hide.
            const char letter = static_cast<char>(type);
            std::string name;
            if (letter >= 0x20 && letter < 0x7f)
                name = std::string(1, letter);
            else
                name = "0x" + HexU64(type).substr(14);
            message_type_histograms_.push_back(Capture(std::move(name), by_type[type]));
        }
    }

    void RunResult::Finish()
    {
        duration_s_ = std::chrono::duration<double>(std::chrono::steady_clock::now() - started_).count();

        for (int cpu_id : pinned_cpus_)
        {
            const std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu_id) + "/cpufreq/scaling_governor";
            host_.AddString("governor.cpu" + std::to_string(cpu_id), ReadTextFile(path));
        }
    }

    std::string RunResult::DefaultPath(const std::string &out_dir) const
    {
        if (out_dir.empty())
            return {};
        const std::string date = started_at_.substr(0, 10);
        return out_dir + "/bench=" + benchmark_ + "/date=" + date + "/" + run_id_ + ".json";
    }

    void RunResult::WriteBuildJson(JsonWriter &writer) const
    {
        writer.BeginObject("build");
        writer.Field("git_sha", BENCH_GIT_SHA);
        writer.Field("git_dirty", BENCH_GIT_DIRTY != 0);
        writer.Field("config", BENCH_BUILD_CONFIG);
        writer.Field("cxxflags", BENCH_CXXFLAGS);
        writer.Field("compiler", __VERSION__);
#ifdef NDEBUG
        writer.Field("assertions", false);
#else
        writer.Field("assertions", true);
#endif
        writer.EndObject();
    }

    void RunResult::WriteHostJson(JsonWriter &writer) const
    {
        writer.BeginObject("host");
        host_.WriteJson(writer);
        writer.EndObject();
    }

    void RunResult::WriteHistogramJson(JsonWriter &writer, const NamedHistogram &histogram) const
    {
        writer.BeginObject(histogram.name);
        writer.Field("samples", histogram.samples);
        writer.Field("total_cycles", histogram.total_cycles);
        writer.Field("clipped", histogram.clipped);
        writer.Field("max_cycles", histogram.max_cycles);
        writer.Field("top_buckets", HDRHistogram::TOP_BUCKETS);
        writer.Field("sub_buckets", HDRHistogram::SUB_BUCKETS);

        // Derived, and derived the same way by tools/report.py from the buckets below. Present so a document can be read at a glance; never the only copy of a number.
        writer.BeginObject("summary_ns");
        const double mean_cycles = histogram.samples == 0
                                       ? 0.0
                                       : static_cast<double>(histogram.total_cycles) / static_cast<double>(histogram.samples);
        writer.Field("mean", mean_cycles * ns_per_cycle_);
        writer.Field("p50", static_cast<double>(histogram.p50) * ns_per_cycle_);
        writer.Field("p90", static_cast<double>(histogram.p90) * ns_per_cycle_);
        writer.Field("p99", static_cast<double>(histogram.p99) * ns_per_cycle_);
        writer.Field("p99_9", static_cast<double>(histogram.p99_9) * ns_per_cycle_);
        writer.Field("p99_99", static_cast<double>(histogram.p99_99) * ns_per_cycle_);
        writer.Field("max", static_cast<double>(histogram.max_cycles) * ns_per_cycle_);
        writer.EndObject();

        // Sparse [bucket index, count], in cycles. Roughly 2000 buckets are reserved and a few dozen are ever occupied, so the whole distribution costs about as much as the summary above -- and every percentile stays recomputable later, at a resolution no printed number preserves.
        writer.BeginArray("buckets", true);
        for (size_t i = 0; i < histogram.buckets.size(); i++)
        {
            if (histogram.buckets[i] == 0)
                continue;
            writer.BeginArray(true);
            writer.Value(i);
            writer.Value(histogram.buckets[i]);
            writer.EndArray();
        }
        writer.EndArray();

        writer.EndObject();
    }

    void RunResult::WriteJson(JsonWriter &writer) const
    {
        writer.BeginObject();
        writer.Field("schema_version", kSchemaVersion);
        writer.Field("run_id", run_id_);
        writer.Field("benchmark", benchmark_);
        writer.Field("status", status_);
        if (!error_.empty())
            writer.Field("error", error_);
        writer.Field("started_at", started_at_);
        writer.Field("started_at_unix_ns", started_at_unix_ns_);
        writer.Field("duration_s", duration_s_);

        WriteBuildJson(writer);
        WriteHostJson(writer);

        if (has_input_)
        {
            writer.BeginObject("input");
            input_.WriteJson(writer);
            writer.EndObject();
        }

        writer.BeginObject("config");
        config_.WriteJson(writer);
        writer.EndObject();

        writer.BeginObject("timing");
        writer.Field("ns_per_cycle", ns_per_cycle_);
        writer.Field("ns_per_cycle_end", ns_per_cycle_end_);
        // The two calibrations bracket the run: a drift far from zero means the clock moved under the measurement and every ns figure in this document is suspect.
        writer.Field("ns_per_cycle_drift", ns_per_cycle_ == 0.0 ? 0.0 : (ns_per_cycle_end_ - ns_per_cycle_) / ns_per_cycle_);
        writer.EndObject();

        writer.BeginObject("metrics");
        metrics_.WriteJson(writer);
        writer.EndObject();

        writer.BeginObject("histograms");
        for (const NamedHistogram &histogram : histograms_)
        {
            WriteHistogramJson(writer, histogram);
        }
        writer.EndObject();

        writer.BeginObject("message_type_histograms");
        for (const NamedHistogram &histogram : message_type_histograms_)
        {
            WriteHistogramJson(writer, histogram);
        }
        writer.EndObject();

        writer.EndObject();
    }

    void RunResult::WriteIndexJson(JsonWriter &writer) const
    {
        writer.BeginObject();
        writer.Field("run_id", run_id_);
        writer.Field("benchmark", benchmark_);
        writer.Field("status", status_);
        writer.Field("error", error_);
        writer.Field("started_at", started_at_);
        writer.Field("duration_s", duration_s_);
        writer.Field("git_sha", BENCH_GIT_SHA);
        writer.Field("git_dirty", BENCH_GIT_DIRTY != 0);
        writer.Field("build_config", BENCH_BUILD_CONFIG);
        writer.Field("ns_per_cycle", ns_per_cycle_);

        // Flat, underscore-separated names: this file is meant to be loaded straight into BigQuery or a dataframe, where a dotted or nested name is a nuisance.
        host_.WriteFlatFields(writer, "host_");
        if (has_input_)
            input_.WriteFlatFields(writer, "input_");
        metrics_.WriteFlatFields(writer, "m_");
        config_.WriteFlatFields(writer, "cfg_");

        for (const NamedHistogram &histogram : histograms_)
        {
            const std::string prefix = "h_" + histogram.name + "_";
            const double mean_cycles = histogram.samples == 0
                                           ? 0.0
                                           : static_cast<double>(histogram.total_cycles) / static_cast<double>(histogram.samples);
            writer.Field(prefix + "samples", histogram.samples);
            writer.Field(prefix + "clipped", histogram.clipped);
            writer.Field(prefix + "mean_ns", mean_cycles * ns_per_cycle_);
            writer.Field(prefix + "p50_ns", static_cast<double>(histogram.p50) * ns_per_cycle_);
            writer.Field(prefix + "p90_ns", static_cast<double>(histogram.p90) * ns_per_cycle_);
            writer.Field(prefix + "p99_ns", static_cast<double>(histogram.p99) * ns_per_cycle_);
            writer.Field(prefix + "p99_9_ns", static_cast<double>(histogram.p99_9) * ns_per_cycle_);
            writer.Field(prefix + "p99_99_ns", static_cast<double>(histogram.p99_99) * ns_per_cycle_);
            writer.Field(prefix + "max_ns", static_cast<double>(histogram.max_cycles) * ns_per_cycle_);
        }

        writer.EndObject();
    }

    ValueMap DefaultOutputOptions()
    {
        ValueMap output;
        output.AddString("out_dir", "results", "root of the run-document tree (empty writes no file)");
        output.AddString("out", "", "exact path for the run document, overriding the out_dir layout");
        output.AddString("index", "results/index.jsonl", "one-line-per-run summary to append to (empty disables)");
        output.AddBool("stdout_json", false, "also write the run document to stdout, on one line");
        output.AddBool("quiet", false, "suppress the human-readable summary on stderr");
        return output;
    }

    bool Emit(const RunResult &result, const ValueMap &output)
    {
        bool ok = true;

        const std::string &explicit_path = output.GetString("out");
        std::string path = explicit_path.empty() ? result.DefaultPath(output.GetString("out_dir")) : explicit_path;

        // The run id is unique down to the second, so two runs of one configuration started inside the same second would land on the same name. An explicit --out is the caller saying where it wants the file; the generated path steps aside instead, because losing a run to a silent overwrite is worse than a suffix.
        if (explicit_path.empty() && !path.empty())
        {
            const std::string stem = path.substr(0, path.size() - 5); // trailing ".json"
            for (int attempt = 2; std::filesystem::exists(path) && attempt < 1000; attempt++)
            {
                path = stem + "-" + std::to_string(attempt) + ".json";
            }
        }

        if (!path.empty())
        {
            std::string error;
            if (!EnsureParentDirectory(path, &error))
            {
                std::cerr << "run result: " << error << std::endl;
                ok = false;
            }
            else
            {
                // Written beside the target and renamed into place, so a bucket sync or a reader never sees a half-written document.
                const std::string temporary = path + ".partial";
                {
                    std::ofstream file(temporary, std::ios::trunc);
                    JsonWriter writer(file, true);
                    result.WriteJson(writer);
                    file.flush();
                    if (!file)
                    {
                        std::cerr << "run result: failed writing " << temporary << std::endl;
                        ok = false;
                    }
                }
                if (ok && std::rename(temporary.c_str(), path.c_str()) != 0)
                {
                    std::cerr << "run result: failed renaming " << temporary << " to " << path << ": " << std::strerror(errno) << std::endl;
                    ok = false;
                }
                if (ok)
                    std::cerr << "run result: " << path << std::endl;
            }
        }

        const std::string &index = output.GetString("index");
        if (!index.empty())
        {
            std::string error;
            if (!EnsureParentDirectory(index, &error))
            {
                std::cerr << "run result: " << error << std::endl;
                ok = false;
            }
            else
            {
                std::ofstream file(index, std::ios::app);
                JsonWriter writer(file, false);
                result.WriteIndexJson(writer);
                file << '\n';
                file.flush();
                if (!file)
                {
                    std::cerr << "run result: failed appending to " << index << std::endl;
                    ok = false;
                }
            }
        }

        if (output.GetBool("stdout_json"))
        {
            JsonWriter writer(std::cout, false);
            result.WriteJson(writer);
            std::cout << std::endl;
        }

        return ok;
    }
}
