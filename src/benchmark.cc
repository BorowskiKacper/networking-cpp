#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <cassert>
#include <iostream>

#include "../include/benchmark.h"

namespace bench
{
    inline void HDRHistogram::Record(uint64_t cycles)
    {
        samples++;
        total_cycles += cycles;

        int zeros = __builtin_clzll(cycles);
        uint64_t top_bucket = TOP_BUCKETS - 1 - zeros;
        uint64_t sub_bucket = (cycles >> (top_bucket - 6)) & 0x3F; // & 0011 1111
        uint64_t bucket = TOP_BUCKETS * top_bucket + sub_bucket;
        if (bucket < TOP_BUCKETS * SUB_BUCKETS)
            buckets[bucket]++;
        else
            clipped++;

        if (cycles > max_cycles_seen)
            max_cycles_seen = cycles;
    }

    void HDRHistogram::Save(std::string file_name)
    {
        std::ofstream outfile(file_name);
        if (outfile.is_open())
        {
            outfile << TOP_BUCKETS << " " << SUB_BUCKETS << " \n";

            for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
            {
                outfile << buckets[i] << '\n';
            }

            outfile << "samples: " << samples << '\n'
                    << "total_cycles: " << total_cycles << '\n'
                    << "clipped: " << clipped << '\n'
                    << "max_cycles_seen: " << max_cycles_seen << '\n';
        }
        else
        {
            std::cerr << "Error saving HDR Histrogram to file";
        }
    }

    uint64_t HDRHistogram::PercentileBucket(double p) const
    {
        int count = 0;
        assert(0 <= p && p <= 1);

        for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
        {
            count += buckets[i];
            if (count >= p * count)
            {
                return i;
            }
        }

        return -1;
    }

    HDRHistogram &HDRHistogram::operator+=(const HDRHistogram other)
    {
        samples += other.samples;
        total_cycles += other.total_cycles;
        if (other.max_cycles_seen > max_cycles_seen)
            max_cycles_seen = other.max_cycles_seen;
        clipped += other.clipped;

        for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
        {
            buckets[i] += other.buckets[i];
        }

        return *this;
    }

    std::ostream &operator<<(std::ostream &os, const HDRHistogram h)
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

    inline uint64_t rdtsc_start()
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

    inline uint64_t rdtsc_end()
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

}