#include "receiver/spsc_buffer.h"

#include <atomic>
#include <new>
#include <cstddef>

namespace fh_lob {
    template <typename T, size_t capacity>
    bool SPSCRingBuffer<T, capacity>::empty() const 
    {
        return consumer_.head.load(std::memory_order_relaxed) == producer_.tail.load(std::memory_order_relaxed);
    }
};