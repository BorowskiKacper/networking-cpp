#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <cassert>

namespace bench
{
    class HDRHistogram
    {
        static constexpr uint32_t TOP_BUCKETS = 64;
        static constexpr uint32_t SUB_BUCKETS = 32;
        uint64_t buckets[TOP_BUCKETS * SUB_BUCKETS];
        uint64_t samples = 0, total_cycles = 0, clipped = 0, max_cycles_seen = 0;

        inline void Record(uint64_t cycles);

        // Stores HDRHistogram in the specified file
        // File structure:
        //  First line specifies the number of top buckets and sub buckets
        //  Lines 1 to TOP_BUCKET*SUB_BUCKETS (inclusive) record the number in each bucket
        //  Last 4 lines (starting at line TOP_BUCKET*SUB_BUCKETS + 1) record samples, total_cycles, clipped, and max_cycles_seen respectively
        void Save(std::string file_name);

        // percentile p must be between 0 and 1.
        // Returns the index of a bucket corresponding to the percentile if found, otherwise returns -1.
        uint64_t PercentileBucket(double p) const;

        // Prints samples, mean, p50, p90, p99, p99.9, max, clipped in ns.
        friend std::ostream &operator<<(std::ostream &os, const HDRHistogram h)
        {
            os << "Samples: " << h.samples
               << "Mean: " << h.total_cycles / h.samples
               << "p50: " << h.PercentileBucket(0.5)
               << "p90: " << h.PercentileBucket(0.9)
               << "p99: " << h.PercentileBucket(0.99)
               << "p99.9: " << h.PercentileBucket(0.999)
               << "max: " << h.max_cycles_seen
               << "clipped: " << h.clipped;

            return os;
        }
    };

    inline uint64_t rdtsc_start();
    inline uint64_t rdtsc_end();
}