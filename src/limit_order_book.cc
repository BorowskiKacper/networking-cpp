#include <cstdint>
#include <vector>
#include <unordered_map>

#include "include/limit_order_book.h"

namespace fh_lob
{
    OrderPool::OrderPool(size_t capacity)
    {
        pool.resize(capacity);
        free_list.reserve(capacity);
        // Push all pointers to the free list
        for (size_t i = 0; i < capacity; i++)
        {
            free_list.push_back(&pool[i]);
        }
    }

    Order *OrderPool::allocate()
    {
        if (free_list.empty())
            return nullptr;
        Order *order = free_list.back();
        free_list.pop_back();
        return order;
    }

    void OrderPool::deallocate(Order *order)
    {
        order->next = nullptr;
        order->prev = nullptr;
        free_list.push_back(order);
    }

    void LimitOrderBook::add_order(uint64_t id, uint64_t price, uint32_t size, bool is_buy)
    {
        Order *order = order_pool.allocate();
        order->id = id;
        order->price = price;
        order->size = size;
        order->is_buy = is_buy;

        order_map[id] = order;

        if (price_map.find(price) == price_map.end())
            price_map[price] = new PriceLevel{price};

        PriceLevel *level = price_map[price];
        level->total_volume += size;

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
    void LimitOrderBook::cancel_order(uint64_t id)
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

        level->total_volume -= order->size;
        if (level->total_volume == 0)
        {
            delete level;
            price_map.erase(order->price);
        }

        order_map.erase(id);
        order_pool.deallocate(order);
    }

    void LimitOrderBook::execute_order(uint64_t id, uint32_t exec_size)
    {
        auto it = order_map.find(id);
        if (it == order_map.end())
            return;

        Order *order = it->second;

        if (order->size >= exec_size)
            cancel_order(id);
        else
        {
            order->size -= exec_size;
            price_map[order->price]->total_volume -= exec_size;
        }
    }

}