#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace fh_lob
{
    struct Order
    {
        uint64_t id;
        uint64_t price;
        uint32_t size;
        bool is_buy;

        Order *next = nullptr;
        Order *prev = nullptr;
    };

    struct PriceLevel
    {
        uint64_t price;
        uint32_t total_volume;

        Order *head = nullptr;
        Order *tail = nullptr;
    };

    template <typename T>
    class MemoryPool
    {
    private:
        std::vector<T> pool;
        std::vector<T *> free_list;

    public:
        MemoryPool(size_t capacity);

        T *allocate();
        void deallocate(T *type);
    };

    class LimitOrderBook
    {
    private:
        MemoryPool<Order> order_pool;
        MemoryPool<PriceLevel> price_level_pool;

        std::unordered_map<uint64_t, Order *> order_map;
        std::unordered_map<uint64_t, PriceLevel *> price_map;

    public:
        LimitOrderBook(size_t max_orders) : order_pool(max_orders), price_level_pool(max_orders) {}

        void add_order(uint64_t id, uint64_t price, uint32_t size, bool is_buy);
        void cancel_order(uint64_t id);
        void execute_order(uint64_t id, uint32_t exec_size);
    };
}