#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <fstream>

namespace bench
{
    class HDRHistogram
    {
    private:
        static constexpr uint32_t TOP_BUCKETS = 32;
        static constexpr uint32_t SUB_BUCKETS = 64;
        uint64_t buckets[TOP_BUCKETS * SUB_BUCKETS]{};
        uint64_t samples = 0, total_cycles = 0, clipped = 0, max_cycles_seen = 0;

        // percentile p must be between 0 and 1.
        // Returns the index of a bucket corresponding to the percentile if found, otherwise returns -1.
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

            if (bucket < TOP_BUCKETS * SUB_BUCKETS)
                buckets[bucket]++;
            else
                clipped++;

            if (cycles > max_cycles_seen)
                max_cycles_seen = cycles;
        }

        // Stores HDRHistogram in the specified file
        // File structure:
        //  First line specifies the number of top buckets and sub buckets
        //  Lines 1 to TOP_BUCKET*SUB_BUCKETS (inclusive) record the number in each bucket
        //  Last 4 lines (starting at line TOP_BUCKET*SUB_BUCKETS + 1) record samples, total_cycles, clipped, and max_cycles_seen respectively
        void Save(std::string file_name);

        HDRHistogram &operator+=(const HDRHistogram other);

        // Prints samples, mean, p50, p90, p99, p99.9, max, clipped in ns.
        friend std::ostream &operator<<(std::ostream &os, const HDRHistogram h);
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
    uint64_t FindNsPerCycle(size_t ms);

    // Pins the current thread to specific cpu
    void pin_to_cpu(int cpu_id);

    extern thread_local HDRHistogram hist[256]; // indexed by ITCH Message Type
}