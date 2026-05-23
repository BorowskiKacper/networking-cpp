#include <cstdint>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "../../include/receiver/limit_order_book.h"

namespace fh_lob
{
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
            // price_level_pool.deallocate(level);
            // price_map.erase(order->price);
        }

        order_map.erase(id);
        order_pool.deallocate(order);
    }

    void LimitOrderBook::ReplaceOrder(uint64_t old_id, uint64_t new_id, uint32_t shares, uint32_t new_price)
    {
        auto it = order_map.find(old_id);
        if (it == order_map.end())
            return; // Order not found

        Order *order = it->second;
        PriceLevel *level = price_map[order->price];

        // REVISIT: is this implementation correct? should I ignore how many shares there were previously or not?
        char side = order->side;
        DeleteOrder(old_id);
        AddOrder(new_id, side, shares, new_price);
        // if (level->price != new_price || order->shares < shares)
        // {
        //     char side = order->side;
        //     DeleteOrder(old_id);
        //     AddOrder(new_id, side, shares, new_price);
        // }
        // else
        // {
        //     ReduceOrderSize(old_id, order->shares - shares);
        //     // replace old_id with new_id
        // }
    }

}