#pragma once

#include <cstdint>
#include <vector>
#include "absl/container/flat_hash_map.h"

namespace fh_lob
{
    struct Order
    {
        Order *next = nullptr;
        Order *prev = nullptr;
        uint64_t id;
        char side;
        uint32_t shares;
        uint32_t price;
    };

    struct PriceLevel
    {
        Order *head = nullptr;
        Order *tail = nullptr;
        uint32_t price;
        uint32_t total_volume;
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

    template <typename T>
    MemoryPool<T>::MemoryPool(size_t capacity)
    {
        pool.resize(capacity);
        free_list.reserve(capacity);
        // Push all pointers to the free list
        for (size_t i = 0; i < capacity; i++)
        {
            free_list.push_back(&pool[i]);
        }
    }

    template <typename T>
    T *MemoryPool<T>::allocate()
    {
        if (free_list.empty())
        {
            size_t chunk_size = 10000;
            T *new_chunk = new T[chunk_size];

            for (size_t i = 0; i < chunk_size; i++)
            {
                free_list.push_back(&new_chunk[i]);
            }

            std::cout << "Memory pool exhausted! Fallback chunk allocated" << std::endl;
        }

        T *type = free_list.back();
        free_list.pop_back();
        return type;
    }

    template <typename T>
    void MemoryPool<T>::deallocate(T *type)
    {
        free_list.push_back(type);
    }

    class LimitOrderBook
    {
    private:
        MemoryPool<Order> order_pool;
        MemoryPool<PriceLevel> price_level_pool;

        absl::flat_hash_map<uint64_t, Order *> order_map;                          // stock locate --> order
        std::vector<absl::flat_hash_map<uint32_t, PriceLevel *>> price_level_maps; // stock locate --> price level map --> price level

        absl::flat_hash_map<std::array<char, 8>, Order *> str_to_locate;
        std::vector<std::array<char, 8>> locate_to_str;

        void ReduceOrderSize(uint64_t id, uint32_t shares);

    public:
        LimitOrderBook(size_t orders, size_t price_levels) : order_pool(orders), price_level_pool(price_levels) {}

        void AddOrder(uint64_t id, char side, uint32_t shares, uint32_t price);
        void ExecuteOrder(uint64_t id, uint32_t shares);
        void ExecuteOrderWithPrice(uint64_t id, uint32_t shares, uint32_t price);
        void CancelOrder(uint64_t id, uint32_t shares);
        void DeleteOrder(uint64_t id);
        void ReplaceOrder(uint64_t old_id, uint64_t new_id, uint32_t shares, uint32_t new_price);

        void MapStrToLocate(std::array<char, 8> &str, uint16_t locate);
        void MapLocateToStr(std::array<char, 8> &str, uint16_t locate);
    };
}