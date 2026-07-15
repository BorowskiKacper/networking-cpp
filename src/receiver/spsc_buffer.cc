#include <atomic>
#include <new>
#include <cstddef>

#include "receiver/spsc_buffer.h"

namespace fh_lob {
    template <typename T, size_t capacity>
    bool SPSCRingBuffer<T, capacity>::empty() const 
    {
        return consumer_.head.load(std::memory_order_relaxed) == producer_.tail.load(std::memory_order_relaxed);
    }
};