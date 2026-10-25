#include "shared/message_parser.h"
#include "shared/limit_order_book.h"
#include "shared/benchmark.h"
#include "shared/run_result.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <chrono>
#include <exception>
#include <string>

namespace
{
    // Everything the replay can be asked to vary. This map is both what the run reads its settings out of and what gets written into the run document, so the recorded configuration cannot drift from the one that ran.
    bench::Config OfflineConfig()
    {
        bench::Config config;
        config.AddString("input", "", "ITCH message file to replay");
        config.AddInt("cpu", 0, "CPU to pin the replay to");
        config.AddUInt("warmup_messages", 10000, "messages parsed before recording starts");
        config.AddUInt("calibration_ms", 50, "sleep length per TSC calibration sample");
        config.AddUInt("lob.order_pool_capacity", 2000000, "live orders the pool can hold");
        config.AddUInt("lob.price_level_pool_capacity", 500ULL * 100 * 10000, "price levels the pool can hold");
        config.AddUInt("lob.max_locates", 10000, "stock locate codes reserved");
        config.AddUInt("lob.max_order_ref_number", 310000000, "size of the order reference table; 12/30/2019 reaches 308,631,072");
        config.AddUInt("lob.expected_levels_per_locate", 256, "price levels reserved per locate");
        return config;
    }

    int RunOffline(const bench::Config &config, bench::RunResult &result, bool quiet)
    {
        const std::string &input_path = config.GetString("input");
        if (input_path.empty())
        {
            result.Fail("no --input file given");
            return EXIT_FAILURE;
        }

        const int cpu_id = static_cast<int>(config.GetInt("cpu"));
        const uint64_t warmup_messages = config.GetUInt("warmup_messages");
        const size_t calibration_ms = static_cast<size_t>(config.GetUInt("calibration_ms"));

        int fd = open(input_path.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
        if (fd < 0)
        {
            result.Fail("could not open " + input_path + ": " + std::strerror(errno));
            return EXIT_FAILURE;
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1)
        {
            result.Fail(std::string("fstat failed: ") + std::strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }

        size_t file_size = static_cast<size_t>(sb.st_size);
        result.SetInputFile(input_path);

        const char *file = static_cast<const char *>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (file == MAP_FAILED)
        {
            result.Fail(std::string("mmap failed: ") + std::strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }

        if (madvise(const_cast<char *>(file), sb.st_size, MADV_SEQUENTIAL) == -1)
        {
            perror("madvise");
        }

        fh_lob::LimitOrderBook lob(config.GetUInt("lob.order_pool_capacity"),
                                   config.GetUInt("lob.price_level_pool_capacity"),
                                   config.GetUInt("lob.max_locates"),
                                   config.GetUInt("lob.max_order_ref_number"),
                                   config.GetUInt("lob.expected_levels_per_locate"));

        bench::pin_to_cpu(cpu_id);
        result.AddPinnedCpu(cpu_id);
        double ns_per_cycle = bench::FindNsPerCycle(calibration_ms);

        const char *cursor = file;
        const char *file_end = file + file_size;

        // Metrics variables
        size_t total_message_count = 0;
        auto start_time = std::chrono::steady_clock::now();

        try
        {
            while (cursor + sizeof(uint16_t) <= file_end)
            {
                uint16_t net_length;
                memcpy(&net_length, cursor, sizeof(net_length));
                const uint16_t length = ntohs(net_length);
                const char *payload = cursor + sizeof(uint16_t);

                uint8_t msg_type = static_cast<uint8_t>(*payload);
                uint64_t start_cycle = bench::RdtscStart();
                fh_lob::ParseMessage(payload, lob);
                uint64_t end_cycle = bench::RdtscEnd();

                if (total_message_count > warmup_messages) // warmup skip
                    bench::hist[msg_type].Record(end_cycle - start_cycle);
                total_message_count++;

                cursor = payload + length;
            }
        }
        catch (const std::exception &e)
        {
            result.Fail("replay aborted at message " + std::to_string(total_message_count) + ": " + e.what());
            munmap(const_cast<char *>(file), file_size);
            close(fd);
            return EXIT_FAILURE;
        }

        // Calculate Metrics
        auto end_time = std::chrono::steady_clock::now();
        auto time_taken = end_time - start_time;

        munmap(const_cast<char *>(file), file_size);
        close(fd);

        // The second calibration brackets the run: if it has drifted from the first, the cycle-to-nanosecond conversion behind every figure below did not hold.
        double second_ns_per_cycle = bench::FindNsPerCycle(calibration_ms);
        result.SetNsPerCycle(ns_per_cycle, second_ns_per_cycle);

        std::chrono::duration<double> elapsed = time_taken;
        const int64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count();

        bench::HDRHistogram overall_hist;
        for (size_t index = 0; index < 256; index++)
        {
            overall_hist += bench::hist[index];
        }

        result.metrics().AddUInt("total_messages", total_message_count, "messages parsed, warmup included");
        result.metrics().AddUInt("recorded_messages", overall_hist.Samples(), "messages whose latency was recorded");
        result.metrics().AddInt("elapsed_ns", elapsed_ns, "wall time of the replay loop");
        result.metrics().AddDouble("throughput_msgs_s", static_cast<double>(total_message_count) / elapsed.count());
        result.metrics().AddDouble("throughput_bytes_s", static_cast<double>(file_size) / elapsed.count());

        result.AddHistogram("overall", overall_hist);
        result.AddMessageTypeHistograms(bench::hist);

        if (!quiet)
        {
            std::cerr << "* Throughput (msgs/sec): " << total_message_count / elapsed.count() << std::endl
                      << "    * Total Time (ns): " << elapsed_ns << std::endl
                      << "    * Total Messages (msgs): " << total_message_count << std::endl;
            overall_hist.PrintSummary(ns_per_cycle);
            std::cerr << "* ns_per_cycle drift over the run: "
                      << (second_ns_per_cycle - ns_per_cycle) / ns_per_cycle << std::endl;
        }

        return EXIT_SUCCESS;
    }
}

int main(int argc, char *argv[])
{
    bench::Config config = OfflineConfig();
    bench::ValueMap output = bench::DefaultOutputOptions();

    // Rendered before parsing, so the defaults it quotes are the defaults and not whatever a half-applied command line left behind.
    const std::string usage = bench::UsageString(argv[0], {&config, &output});
    if (bench::HelpRequested(argc, argv))
    {
        std::cout << usage;
        return EXIT_SUCCESS;
    }

    std::string error;
    if (!bench::ParseArgs(argc, argv, {&config, &output}, &error))
    {
        std::cerr << argv[0] << ": " << error << "\n\n"
                  << usage;
        return EXIT_FAILURE;
    }

    bench::RunResult result("offline_lob", config);
    const int status = RunOffline(config, result, output.GetBool("quiet"));
    result.Finish();

    // The document is written whether or not the run succeeded: a failed run is a record, not a gap in the series.
    if (!bench::Emit(result, output))
        return EXIT_FAILURE;

    return status;
}
