#pragma once

#include <arpa/inet.h>

static inline uint64_t rdtsc_start()
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

static inline uint64_t rdtsc_end()
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