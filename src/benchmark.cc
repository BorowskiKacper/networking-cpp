#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

#include "../include/benchmark.h"

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

    uint64_t FindNsPerCycle(size_t ms)
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