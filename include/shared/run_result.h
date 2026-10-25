#pragma once

#include "shared/value_map.h"

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace bench
{
    class HDRHistogram;
    class JsonWriter;

    // The configuration a benchmark run was asked for. Same type as any other value map; named separately because it is the half of the command line that identifies the run.
    using Config = ValueMap;

    // One benchmark run, as a single self-contained JSON document: what was built, where it ran, what it was configured with, what it measured, and the raw histogram buckets behind every number. Nothing here is printed for a human to parse back -- stdout stays a courtesy, and this document is the interface.
    //
    // Histograms are copied in, so a caller may hand over a local and let it die.
    class RunResult
    {
    public:
        // Captures the wall clock, the build provenance and the host description up front, and derives the run id from the timestamp, the commit and a hash of the configuration.
        RunResult(std::string benchmark, const Config &config);

        // Marks the run as failed. The document is still written: a crashed run is a record, not a gap in the series.
        void Fail(std::string message);
        bool ok() const { return status_ == "ok"; }

        // Records the two TSC calibrations, which bracket the run and whose difference is the only evidence that the cycle-to-nanosecond conversion held.
        void SetNsPerCycle(double start, double end);

        // Adds the pinned CPUs so the host block can report their frequency governors.
        void AddPinnedCpu(int cpu_id);

        // Size, mtime and a cheap content fingerprint of the replayed file. The fingerprint covers the length plus the first and last 64 KiB -- enough to notice a different input, without a second pass over gigabytes that would also disturb the page cache the benchmark depends on.
        void SetInputFile(const std::string &path);

        ValueMap &metrics() { return metrics_; }
        const ValueMap &metrics() const { return metrics_; }

        void AddHistogram(std::string name, const HDRHistogram &histogram);
        // Emits the non-empty entries of a 256-wide, message-type-indexed array under their ITCH type letter.
        void AddMessageTypeHistograms(const HDRHistogram *by_type);

        // Stamps the duration. Call once, after the run and before writing.
        void Finish();

        const std::string &run_id() const { return run_id_; }
        const std::string &benchmark() const { return benchmark_; }

        // <out_dir>/bench=<benchmark>/date=<YYYY-MM-DD>/<run_id>.json -- the same layout locally and in a bucket, so publishing is a recursive copy and nothing has to be rewritten on the way.
        std::string DefaultPath(const std::string &out_dir) const;

        void WriteJson(JsonWriter &writer) const;
        // The flattened one-line form appended to the index file.
        void WriteIndexJson(JsonWriter &writer) const;

    private:
        struct NamedHistogram
        {
            std::string name;
            // Held by value: histograms are ~16 KiB and copying them removes every question about what outlives what.
            std::vector<uint64_t> buckets;
            uint64_t samples = 0;
            uint64_t total_cycles = 0;
            uint64_t clipped = 0;
            uint64_t max_cycles = 0;
            uint64_t p50 = 0, p90 = 0, p99 = 0, p99_9 = 0, p99_99 = 0;
        };

        NamedHistogram Capture(std::string name, const HDRHistogram &histogram) const;
        void WriteHistogramJson(JsonWriter &writer, const NamedHistogram &histogram) const;
        void WriteBuildJson(JsonWriter &writer) const;
        void WriteHostJson(JsonWriter &writer) const;

        std::string benchmark_;
        std::string run_id_;
        std::string status_ = "ok";
        std::string error_;

        Config config_;
        ValueMap metrics_;
        ValueMap host_;

        std::string started_at_;
        int64_t started_at_unix_ns_ = 0;
        std::chrono::steady_clock::time_point started_;
        double duration_s_ = 0.0;

        double ns_per_cycle_ = 0.0;
        double ns_per_cycle_end_ = 0.0;

        bool has_input_ = false;
        ValueMap input_;

        std::vector<int> pinned_cpus_;
        std::vector<NamedHistogram> histograms_;
        std::vector<NamedHistogram> message_type_histograms_;
    };

    // Where a run writes, kept out of the configuration so that two runs differing only in their output paths are still recognisably the same configuration.
    ValueMap DefaultOutputOptions();

    // Writes the document to every configured destination: the run file (created atomically, so a bucket sync never picks up a half-written object), the index line, and optionally one compact line on stdout for log-scraping on Batch. Returns false and explains on stderr if any write failed.
    bool Emit(const RunResult &result, const ValueMap &output);
}
