#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>

namespace fh_lob
{
    inline constexpr std::size_t kCacheLineSize = 64;

    // Intel's L2 spatial prefetcher pulls cache lines in aligned pairs, so two
    // atomics only 64 B apart can still ping-pong between cores. Keep the
    // producer's and the consumer's state 128 B apart to stop that.
    inline constexpr std::size_t kFalseSharingRange = 128;

    // Lock-free single-producer / single-consumer ring buffer.
    //
    // Exactly one thread may call the producer half (ClaimWrite/CommitWrite, ClaimContiguous/CommitBatch) and exactly one other thread the consumer half (PeekRead/CommitRead). No external synchronisation is needed between the two.
    //
    // Slots are claimed in place rather than pushed by value, so the producer can write straight into the ring -- recvmmsg() filling packet slots, say -- without an intermediate copy. A claimed slot stays private to the producer until it is committed.
    //
    // head/tail count cumulatively and are masked only when indexing, so tail - head is the exact queue depth and all `capacity` slots are usable (the classic masked-index version has to leave one slot empty to tell "full" from "empty"). They are 64-bit and monotonic.
    template <typename T, size_t capacity>
    class SPSCRingBuffer
    {
    private:
        static_assert(((capacity - 1) & capacity) == 0, "Capacity must be a power of two");
        static_assert(capacity >= 2, "Capacity must be at least 2");
        static constexpr size_t mask = capacity - 1;

        // cached_head / cached_tail hold the last value each side read from the other's counter. The common case is answered from the cache, so the peer's cache line is only touched when the ring looks full (producer) or empty (consumer).
        struct alignas(kFalseSharingRange) ProducerBlock
        {
            std::atomic<size_t> tail{0};
            size_t cached_head{0};
        };
        struct alignas(kFalseSharingRange) ConsumerBlock
        {
            std::atomic<size_t> head{0};
            size_t cached_tail{0};
        };

        ProducerBlock producer_;
        ConsumerBlock consumer_;
        alignas(kCacheLineSize) T buffer_[capacity];
        char padding_[kFalseSharingRange]; // keeps whatever follows off the last slot's line

    public:
        // Returns the next writable slot, or nullptr if the ring is full. The slot is not visible to the consumer until CommitWrite().
        T *ClaimWrite()
        {
            const size_t tail = producer_.tail.load(std::memory_order_relaxed);
            if (tail - producer_.cached_head == capacity)
            {
                producer_.cached_head = consumer_.head.load(std::memory_order_acquire);
                if (tail - producer_.cached_head == capacity)
                {
                    return nullptr;
                }
            }
            return &buffer_[tail & mask];
        }

        // Publishes the slot handed out by the preceding ClaimWrite().
        void CommitWrite()
        {
            CommitBatch(1);
        }

        // Hands out a run of consecutive writable slots starting at *out, capped at max, at the free space, and at the end of the array -- the run never wraps. Returns the run length; 0 means the ring is full. Commit with CommitBatch(n) for any n up to the returned length, so a short read commits only what was actually filled.
        size_t ClaimContiguous(size_t max, T **out)
        {
            const size_t tail = producer_.tail.load(std::memory_order_relaxed);
            size_t free_slots = capacity - (tail - producer_.cached_head);
            if (free_slots == 0)
            {
                producer_.cached_head = consumer_.head.load(std::memory_order_acquire);
                free_slots = capacity - (tail - producer_.cached_head);
                if (free_slots == 0)
                {
                    return 0;
                }
            }

            const size_t index = tail & mask;
            *out = &buffer_[index];
            return std::min({max, free_slots, capacity - index});
        }

        // Publishes the first n slots of the run handed out by ClaimContiguous().
        void CommitBatch(size_t n)
        {
            const size_t tail = producer_.tail.load(std::memory_order_relaxed);
            producer_.tail.store(tail + n, std::memory_order_release);
        }

        // Index of the next slot the producer will write. Use it to address structures that mirror the ring one-for-one, such as a prebuilt mmsghdr array.
        size_t WriteIndex() const
        {
            return producer_.tail.load(std::memory_order_relaxed) & mask;
        }

        // Base of the slot array, for building those mirror structures at start-up. It does not grant access to slots the caller has not claimed.
        T *Slots()
        {
            return buffer_;
        }

        // Returns the oldest committed, unread slot, or nullptr if the ring is empty. The slot stays valid until CommitRead().
        const T *PeekRead()
        {
            const size_t head = consumer_.head.load(std::memory_order_relaxed);
            if (head == consumer_.cached_tail)
            {
                consumer_.cached_tail = producer_.tail.load(std::memory_order_acquire);
                if (head == consumer_.cached_tail) {
                    return nullptr;
                }
            }
            return &buffer_[head & mask];
        }

        // Releases the slot handed out by the preceding PeekRead() back to the producer.
        void CommitRead()
        {
            const size_t head = consumer_.head.load(std::memory_order_relaxed);
            consumer_.head.store(head + 1, std::memory_order_release);
        }

        // Committed, unread slots. Callable from either thread, but it reads the peer's counter, so keep it off the per-item path -- once per batch is fine, once per packet is not.
        size_t Depth() const
        {
            const size_t tail = producer_.tail.load(std::memory_order_acquire);
            const size_t head = consumer_.head.load(std::memory_order_acquire);
            return tail - head;
        }

        bool Empty() const
        {
            return Depth() == 0;
        }
    };
}
