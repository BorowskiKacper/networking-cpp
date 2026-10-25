#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace bench
{
    class HDRHistogram
    {
    public:
        static constexpr uint32_t TOP_BUCKETS = 32;
        static constexpr uint32_t SUB_BUCKETS = 64;
        static constexpr uint32_t BUCKET_COUNT = TOP_BUCKETS * SUB_BUCKETS;

    private:
        uint64_t buckets[BUCKET_COUNT]{};
        uint64_t samples = 0, total_cycles = 0, clipped = 0, max_cycles_seen = 0;

        // Percentile p must be between 0 and 1. Returns the index of the bucket corresponding to the percentile. If clipped samples push the percentile beyond the histogram's range, returns the last bucket index.
        uint64_t PercentileBucket(double p) const;

    public:
        inline void Record(uint64_t cycles)
        {
            samples++;
            total_cycles += cycles;

            uint32_t bit_position = 63 - __builtin_clzll(cycles | 1); // treat cycles=0 as 1 to avoid undefined behavior.
            uint32_t shift = bit_position < 6 ? 0 : bit_position - 6;
            uint64_t sub_bucket = (cycles >> shift) & 0x3F;
            uint64_t top_bucket = bit_position < 6 ? 0 : bit_position - 5;
            uint64_t bucket = (top_bucket << 6) | sub_bucket;

            if (bucket < BUCKET_COUNT)
                buckets[bucket]++;
            else
                clipped++;

            if (cycles > max_cycles_seen)
                max_cycles_seen = cycles;
        }

        HDRHistogram &operator+=(const HDRHistogram &other);

        // Writes samples, mean, p50, p90, p99, p99.9, max and clipped to stderr, in ns.
        void PrintSummary(double ns_per_cycle) const;

        // Returns mid-bucket cycle value given the bucket
        uint64_t BucketToCycles(uint64_t bucket) const;

        // Mid-bucket cycle value at percentile p (0..1).
        uint64_t PercentileCycles(double p) const;

        uint64_t Samples() const { return samples; }
        uint64_t TotalCycles() const { return total_cycles; }
        uint64_t Clipped() const { return clipped; }
        uint64_t MaxCycles() const { return max_cycles_seen; }
        bool Empty() const { return samples == 0 && clipped == 0; }

        // Raw counts, so a run result can carry the histogram itself rather than a handful of percentiles distilled from it.
        const uint64_t *Buckets() const { return buckets; }
    };

    inline uint64_t RdtscStart()
    {
        uint32_t low, high;
        asm volatile(
            "lfence\n\t"
            "rdtsc\n\t"
            : "=a"(low), "=d"(high)
            :
            : "memory");
        return ((uint64_t)high << 32) | low;
    }

    inline uint64_t RdtscEnd()
    {
        uint32_t low, high;
        asm volatile(
            "rdtscp\n\t"
            "lfence"
            : "=a"(low), "=d"(high)
            :
            : "memory", "rcx");
        return ((uint64_t)high << 32) | low;
    }

    // Returns median of 5 tests separated by 'ms' each
    double FindNsPerCycle(size_t ms);

    // Pins the current thread to specific cpu
    void pin_to_cpu(int cpu_id);

    extern thread_local HDRHistogram hist[256]; // indexed by ITCH Message Type
}
