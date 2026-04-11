#include <cstdint>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "include/limit_order_book.h"

namespace fh_lob
{
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

    void LimitOrderBook::ReduceOrderSize(uint64_t id, uint32_t shares)
    {
        auto it = order_map.find(id);
        if (it == order_map.end())
            return;

        Order *order = it->second;

        if (order->shares <= shares)
            DeleteOrder(id);
        else
        {
            order->shares -= shares;
            price_map[order->price]->total_volume -= shares;
        }
    }

    void LimitOrderBook::AddOrder(uint64_t id, char side, uint32_t shares, uint32_t price)
    {
        Order *order = order_pool.allocate();
        order->id = id;
        order->side = side;
        order->shares = shares;
        order->price = price;

        order->next = nullptr;
        order->prev = nullptr;

        order_map[id] = order;

        PriceLevel *level;
        if (price_map.find(price) == price_map.end())
        {
            level = price_level_pool.allocate();
            level->price = price;
            price_map[price] = level;
        }
        else
        {
            level = price_map[price];
        }

        level->total_volume += shares;

        if (!level->head)
        {
            level->head = order;
            level->tail = order;
        }
        else
        {
            level->tail->next = order;
            order->prev = level->tail;
            level->tail = order;
        }
    }
    void LimitOrderBook::ExecuteOrder(uint64_t id, uint32_t shares)
    {
        ReduceOrderSize(id, shares);
    }

    void LimitOrderBook::ExecuteOrderWithPrice(uint64_t id, uint32_t shares, uint32_t price)
    {
        ReduceOrderSize(id, shares);
    }

    void LimitOrderBook::CancelOrder(uint64_t id, uint32_t shares)
    {
        ReduceOrderSize(id, shares);
    }

    void LimitOrderBook::DeleteOrder(uint64_t id)
    {
        auto it = order_map.find(id);
        if (it == order_map.end())
            return; // Order not found

        Order *order = it->second;
        PriceLevel *level = price_map[order->price];

        if (order->next)
            order->next->prev = order->prev;
        else
            level->tail = order->prev;
        if (order->prev)
            order->prev->next = order->next;
        else
            level->head = order->next;

        level->total_volume -= order->shares;
        if (level->total_volume == 0)
        {
            price_level_pool.deallocate(level);
            price_map.erase(order->price);
        }

        order_map.erase(id);
        order_pool.deallocate(order);
    }

}