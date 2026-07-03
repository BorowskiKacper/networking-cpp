#pragma once

#include <cstdint>
#include <vector>
#include "absl/container/flat_hash_map.h"

namespace fh_lob
{
    template <typename T>
    class MemoryPool
    {
    private:
        std::vector<T> pool;
        std::vector<uint32_t> free_list;

    public:
        static constexpr uint32_t NIL = UINT32_MAX;
        MemoryPool(uint32_t capacity)
        {
            pool.resize(capacity);
            free_list.reserve(capacity);
            for (uint32_t i = capacity; i-- > 0;)
            {
                free_list.push_back(i);
            }
        }

        uint32_t allocate_index();
        void deallocate_index(uint32_t index);
        T *get(uint32_t index);
    };

    template <typename T>
    uint32_t MemoryPool<T>::allocate_index()
    {
        if (free_list.empty())
            throw std::runtime_error("pool exhausted");
        uint32_t index = free_list.back();
        free_list.pop_back();
        return index;
    }

    template <typename T>
    void MemoryPool<T>::deallocate_index(uint32_t index)
    {
        free_list.push_back(index);
    }

    template <typename T>
    T *MemoryPool<T>::get(uint32_t index)
    {
        return &pool[index];
    }

    struct Order
    {
        uint32_t next = MemoryPool<Order>::NIL;
        uint32_t prev = MemoryPool<Order>::NIL;
        char side;
        uint32_t shares;
        uint32_t price;
    };

    struct PriceLevel
    {
        uint32_t head = MemoryPool<Order>::NIL;
        uint32_t tail = MemoryPool<Order>::NIL;
        uint32_t price;
        uint32_t total_volume;
    };

    class LimitOrderBook
    {
    private:
        MemoryPool<Order> order_pool;
        MemoryPool<PriceLevel> price_level_pool;

        std::vector<uint32_t> orders;                                              // stock locate --> order
        std::vector<absl::flat_hash_map<uint32_t, PriceLevel *>> price_level_maps; // stock locate --> price level map --> price level

        absl::flat_hash_map<uint64_t, uint16_t> str_to_locate;
        std::vector<uint64_t> locate_to_str;

        void ReduceOrderSize(uint16_t locate, uint64_t id, uint32_t shares);

    public:
        LimitOrderBook(size_t order_pool, size_t price_level_pool, size_t locates, size_t max_order_ref_number) : order_pool(order_pool), price_level_pool(price_level_pool)
        {
            price_level_maps.resize(locates);
            locate_to_str.resize(locates);
            str_to_locate.reserve(locates);
            orders.resize(max_order_ref_number, MemoryPool<Order>::NIL);
        }

        void AddOrder(uint16_t locate, uint64_t order_ref_number, char side, uint32_t shares, uint32_t price);
        void ExecuteOrder(uint16_t locate, uint64_t order_ref_number, uint32_t shares);
        void ExecuteOrderWithPrice(uint16_t locate, uint64_t order_ref_number, uint32_t shares, uint32_t price);
        void CancelOrder(uint16_t locate, uint64_t order_ref_number, uint32_t shares);
        void DeleteOrder(uint16_t locate, uint64_t order_ref_number);
        void ReplaceOrder(uint16_t locate, uint64_t old_order_ref_number, uint64_t new_order_ref_number, uint32_t shares, uint32_t new_price);

        void MapStockStr(uint64_t stock, uint16_t locate);
    };
}