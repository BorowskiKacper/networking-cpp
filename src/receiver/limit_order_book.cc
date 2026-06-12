#include <cstdint>
#include <vector>
#include "absl/container/flat_hash_map.h"
#include <iostream>

#include "receiver/limit_order_book.h"

namespace fh_lob
{
    void LimitOrderBook::ReduceOrderSize(uint16_t locate, uint64_t id, uint32_t shares)
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

    void LimitOrderBook::AddOrder(uint16_t locate, uint64_t id, char side, uint32_t shares, uint32_t price)
    {
        Order *order = order_pool.allocate();
        order->id = id;
        order->side = side;
        order->shares = shares;
        order->price = price;

        order->next = nullptr;
        order->prev = nullptr;

        order_map[id] = order;

        absl::flat_hash_map<uint32_t, PriceLevel *> &price_map = price_level_maps[locate];
        auto [it, inserted] = price_map.try_emplace(price, nullptr);
        if (inserted)
        {
            it->second = price_level_pool.allocate();
            it->second->price = price;
            it->second->total_volume = 0;
        }

        PriceLevel *level = it->second;
        level->total_volume += shares;

        if (inserted)
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

    void LimitOrderBook::ExecuteOrder(uint16_t locate, uint64_t id, uint32_t shares)
    {
        ReduceOrderSize(id, shares);
    }

    void LimitOrderBook::ExecuteOrderWithPrice(uint16_t locate, uint64_t id, uint32_t shares, uint32_t price)
    {
        ReduceOrderSize(id, shares);
        price = price; // handle warning
    }

    void LimitOrderBook::CancelOrder(uint16_t locate, uint64_t id, uint32_t shares)
    {
        ReduceOrderSize(id, shares);
    }

    void LimitOrderBook::DeleteOrder(uint16_t locate, uint64_t id)
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
        // PriceLevel *level = price_map[order->price];

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

    void LimitOrderBook::MapStockStr(uint64_t stock, uint16_t locate)
    {
        size_t size = locate_to_str.size();
        while (locate >= size)
        {
            std::cout << "LimitOrderBook::MapStockStr WARNING: size (" << size << ") of locate_to_str is smaller than locate (" << locate << "). Resizing locate_to_str and price_level_maps to double capacity. This indicates that the number of locates exceeds the expected number of locates." << std::endl;
            size *= 2;
            locate_to_str.resize(size);
            price_level_maps.resize(size);
        }
        str_to_locate.try_emplace(stock, locate);
        locate_to_str[locate] = stock;
    }

}