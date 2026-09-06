#include "shared/benchmark.h"

#include <arpa/inet.h>

#include <cstring>
#include <fstream>
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

namespace bench
{
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
        uint64_t count = 0;
        assert(0 <= p && p <= 1);

        for (size_t i = 0; i < TOP_BUCKETS * SUB_BUCKETS; i++)
        {
            count += buckets[i];
            if (count >= p * samples)
            {
                return i;
            }
        }

        return -1;
    }

    HDRHistogram &HDRHistogram::operator+=(const HDRHistogram &other)
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

    uint64_t HDRHistogram::BucketToCycles(uint64_t bucket) const
    {
        uint64_t sub_bucket = bucket & 0x3F;
        uint64_t top_bucket = bucket >> 6;
        if (top_bucket == 0)
            return sub_bucket;

        uint32_t bit_position = top_bucket + 5;

        uint64_t significant_digit = 1ULL << bit_position;
        uint64_t precision_digits = sub_bucket << (bit_position - 6);
        uint64_t mid_bucket = top_bucket <= 1 ? 0 : 1ULL << (bit_position - 7);

        return significant_digit | precision_digits | mid_bucket;
    }

    void HDRHistogram::PrintSummary(double ns_per_cycle)
    {

        std::cout << "* Processing Time per Message (ns/msg): " << total_cycles * ns_per_cycle / static_cast<double>(samples) << std::endl
                  << "* total (msgs): " << samples << std::endl
                  << "* ns_per_cycle (ns/cycle): " << ns_per_cycle << std::endl
                  << "* HDR Histogram (ns/msg): " << std::endl
                  << "    * p50: " << BucketToCycles(PercentileBucket(0.5)) * ns_per_cycle << std::endl
                  << "    * p90: " << BucketToCycles(PercentileBucket(0.9)) * ns_per_cycle << std::endl
                  << "    * p99: " << BucketToCycles(PercentileBucket(0.99)) * ns_per_cycle << std::endl
                  << "    * p99.9: " << BucketToCycles(PercentileBucket(0.999)) * ns_per_cycle << std::endl
                  << "    * max: " << max_cycles_seen * ns_per_cycle << std::endl
                  << "    * clipped (msgs): " << clipped << std::endl;
    }

    double FindNsPerCycle(size_t ms)
    {
        double slopes[5];

        for (size_t i = 0; i < 5; i++)
        {
            timespec ts_start;
            timespec ts_end;

            clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);
            uint64_t start_cycles = RdtscStart();

            std::this_thread::sleep_for(std::chrono::milliseconds(ms));

            clock_gettime(CLOCK_MONOTONIC_RAW, &ts_end);
            uint64_t end_cycles = RdtscEnd();

            slopes[i] = static_cast<double>(ts_end.tv_nsec - ts_start.tv_nsec) / (end_cycles - start_cycles);
        }

        std::sort(slopes, slopes + 5);
        std::cout << "Test slopes" << std::endl; // remove test
        for (int i = 0; i < 5; i++)
        {
            std::cout << "\tSlope " << i << ": " << slopes[i] << std::endl;
        } // remove till here

        return slopes[2];
    }

    void pin_to_cpu(int cpu_id)
    {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu_id, &set);

        if (sched_setaffinity(0, sizeof(set), &set) != 0)
        {
            std::cerr << "sched_setaffinity failed " << std::strerror(errno) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    thread_local bench::HDRHistogram hist[256];
}