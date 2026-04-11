#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace fh_lob
{
    struct Order
    {
        uint64_t id;
        char side;
        uint32_t shares;
        uint32_t price;

        Order *next = nullptr;
        Order *prev = nullptr;
    };

    struct PriceLevel
    {
        uint32_t price;
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
        std::unordered_map<uint32_t, PriceLevel *> price_map;

        void ReduceOrderSize(uint64_t id, uint32_t shares);

    public:
        LimitOrderBook(size_t max_orders) : order_pool(max_orders), price_level_pool(max_orders) {}

        void AddOrder(uint64_t id, char side, uint32_t shares, uint32_t price);
        void ExecuteOrder(uint64_t id, uint32_t shares);
        void ExecuteOrderWithPrice(uint64_t id, uint32_t shares, uint32_t price);
        void CancelOrder(uint64_t id, uint32_t shares);
        void DeleteOrder(uint64_t id);
        void ReplaceOrder(uint64_t old_id, uint64_t new_id, uint32_t shares, uint32_t new_price);
    };
}