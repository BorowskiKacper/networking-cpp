#include <atomic>
#include <new>
#include <cstddef>

namespace fh_lob 
{
    template <typename T, size_t capacity>
    class SPSCRingBuffer 
    {
    private:
        static_assert(((capacity - 1) & capacity) == 0, "Capacity must be a power of two");
        static constexpr size_t mask = capacity - 1;

        struct alignas(std::hardware_destructive_interference_size) ProducerBlock 
        {
            std::atomic<size_t> tail{0};
            size_t cached_head{0};
        };
        struct alignas(std::hardware_destructive_interference_size) ConsumerBlock 
        {
            std::atomic<size_t> head{0};
            size_t cached_tail{0};
        };

        ProducerBlock producer_;
        ConsumerBlock consumer_;
        alignas(std::hardware_destructive_interference_size) T buffer_[capacity];

    public:
        bool push(const T& item) {
            const size_t tail = producer_.tail.load(std::memory_order_relaxed);
            const size_t next = (tail + 1) & mask;
            if(next == producer_.cached_head) 
            {
                producer_.cached_head = consumer_.head.load(std::memory_order_acquire);
                if(next == producer_.cached_head)
                {
                    return false;
                }
            }
            buffer_[tail] = item;
            producer_.tail.store(next, std::memory_order_release);
            return true;
        }

        bool pop(const T& out) 
        {
            const size_t head = consumer_.head.load(std::memory_order_relaxed);
            if(head == consumer_.cached_tail)
            {
                consumer_.cached_tail = producer_.tail.load(std::memory_order_acquire);
                if(head == consumer_.cached_tail) {
                    return false;
                }
            }
            out = buffer_[head];
            consumer_.head.store((head + 1) & mask, std::memory_order_release);
            return true;
        }

        bool empty() const;
    };
}