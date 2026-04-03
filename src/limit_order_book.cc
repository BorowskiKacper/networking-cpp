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
        {
            price_map[price] = new PriceLevel{price};
        }

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
        PriceLevel level = price_map[order->price];
    }
    void LimitOrderBook::execute_order(uint64_t id, uint32_t exec_size);

}