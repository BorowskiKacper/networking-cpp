#include <cstdint>
#include <vector>
#include "absl/container/flat_hash_map.h"
#include <iostream>

#include "receiver/limit_order_book.h"

namespace fh_lob
{
    void LimitOrderBook::ReduceOrderSize(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        uint32_t order_index;
        Order *order;
        order_index = orders[order_ref_number];
        if (order_index == MemoryPool<Order>::NIL)
            return;
        order = order_pool.get(order_index);

        if (order->shares <= shares)
            DeleteOrder(locate, order_ref_number);
        else
        {
            order->shares -= shares;
            absl::flat_hash_map<uint32_t, PriceLevel *> &price_level_map = price_level_maps[locate];
            price_level_map[order->price]->total_volume -= shares;
        }
    }

    void LimitOrderBook::AddOrder(uint16_t locate, uint64_t order_ref_number, char side, uint32_t shares, uint32_t price)
    {
        uint32_t order_index = order_pool.allocate_index();
        Order *order = order_pool.get(order_index);
        order->side = side;
        order->shares = shares;
        order->price = price;

        order->next = MemoryPool<Order>::NIL;
        order->prev = MemoryPool<Order>::NIL;

        orders[order_ref_number] = order_index;

        absl::flat_hash_map<uint32_t, PriceLevel *> &price_level_map = price_level_maps[locate];
        auto [it, inserted] = price_level_map.try_emplace(price, nullptr);
        if (inserted)
        {
            it->second = price_level_pool.get(price_level_pool.allocate_index());
            it->second->price = price;
            it->second->total_volume = 0;
        }

        PriceLevel *level = it->second;
        level->total_volume += shares;

        if (level->head == MemoryPool<Order>::NIL)
        {
            level->head = order_index;
            level->tail = order_index;
        }
        else
        {
            order_pool.get(level->tail)->next = order_index;
            order->prev = level->tail;
            level->tail = order_index;
        }
    }

    void LimitOrderBook::ExecuteOrder(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        ReduceOrderSize(locate, order_ref_number, shares);
    }

    void LimitOrderBook::ExecuteOrderWithPrice(uint16_t locate, uint64_t order_ref_number, uint32_t shares, [[maybe_unused]] uint32_t price)
    {
        ReduceOrderSize(locate, order_ref_number, shares);
    }

    void LimitOrderBook::CancelOrder(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        ReduceOrderSize(locate, order_ref_number, shares);
    }

    void LimitOrderBook::DeleteOrder(uint16_t locate, uint64_t order_ref_number)
    {
        uint32_t order_index;
        Order *order;
        order_index = orders[order_ref_number];
        if (order_index == MemoryPool<Order>::NIL)
            return;
        order = order_pool.get(order_index);

        absl::flat_hash_map<uint32_t, PriceLevel *> &price_level_map = price_level_maps[locate];
        PriceLevel *level = price_level_map[order->price];

        if (order->next != MemoryPool<Order>::NIL)
            order_pool.get(order->next)->prev = order->prev;
        else
            level->tail = order->prev;
        if (order->prev != MemoryPool<Order>::NIL)
            order_pool.get(order->prev)->next = order->next;
        else
            level->head = order->next;

        level->total_volume -= order->shares;
        if (level->total_volume == 0)
        {
            // price_level_pool.deallocate(level);
            // price_level_map.erase(order->price);
        }

        orders[order_ref_number] = MemoryPool<Order>::NIL;
        order_pool.deallocate_index(order_index);
    }

    void LimitOrderBook::ReplaceOrder(uint16_t locate, uint64_t old_order_ref_number, uint64_t new_order_ref_number, uint32_t shares, uint32_t new_price)
    {
        uint32_t order_index;
        Order *order;
        order_index = orders[old_order_ref_number];
        if (order_index == MemoryPool<Order>::NIL)
            return;
        order = order_pool.get(order_index);

        char side = order->side;
        DeleteOrder(locate, old_order_ref_number);
        AddOrder(locate, new_order_ref_number, side, shares, new_price);
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