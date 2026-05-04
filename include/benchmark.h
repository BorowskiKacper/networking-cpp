#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
// #include <iostream>

namespace bench
{
    struct HDRHistogram
    {
        static constexpr uint32_t TOP_BUCKETS = 64;
        static constexpr uint32_t SUB_BUCKETS = 32;
        int *buckets = new int[TOP_BUCKETS * SUB_BUCKETS];
        uint64_t count = 0, sum = 0, clipped = 0, max_seen = 0;

        inline void Record(uint64_t cycles)
        {
            int zeros = __builtin_clzll(cycles);
            int top_bucket = TOP_BUCKETS - 1 - zeros;
            int sub_bucket = (cycles >> (top_bucket - 6)) & 0x3F; // & 0011 1111
            buckets[TOP_BUCKETS * top_bucket + sub_bucket]++;
        }

        void Save(std::string file_name)
        {
            std::ofstream outfile(file_name);
            if (outfile.is_open())
            {
                outfile << TOP_BUCKETS << " " << SUB_BUCKETS << " \n";

                for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
                {
                    outfile << buckets[i] << '\n';
                }

                outfile << "count: " << count << '\n'
                        << "sum: " << sum << '\n'
                        << "clipped: " << clipped << '\n'
                        << "max_seen: " << max_seen << '\n';
            }
            else
            {
                std::cerr << "Error saving HDR Histrogram to file";
            }
        }

        uint64_t N()
        {
            uint64_t n = 0;
            for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
            {
                n += buckets[i];
            }
            return n;
        }
        uint64_t Percentile(uint64_t n, double p) // where 0 <= p <= 1.0
        {
            size_t num_percentiles = 4;
            double percentiles[num_percentiles] = {0.9, 0.99, 0.999, 0.9999};
            int percentile_i = 0;
            int count = 0;

            for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
            {
                count += buckets[i];
                if (count >= percentiles[percentile_i] * n)
                {
                    std::cout << "Percentile" << percentiles[percentile_i] << " count: " << count << std::endl;
                }
            }
        }
    };

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