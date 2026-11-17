#include "shared/limit_order_book.h"

#include "absl/container/flat_hash_map.h"

#include <cstdint>
#include <vector>
#include <iostream>

namespace fh_lob
{
    void LimitOrderBook::ReduceOrderSize(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        Order *order;
        if (use_indexed_orders)
        {
            order = &indexed_orders[order_ref_number];
            if (order->level == MemoryPool<PriceLevel>::NIL)
                return;
        }
        else
        {
            uint32_t order_index = order_indices[order_ref_number];
            if (order_index == MemoryPool<Order>::NIL)
                return;
            order = order_pool.get(order_index);
        }

        if (order->shares <= shares)
            DeleteOrder(locate, order_ref_number);
        else
        {
            order->shares -= shares;
            price_level_pool.get(order->level)->total_volume -= shares;
        }
    }

    void LimitOrderBook::AddOrder(uint16_t locate, uint64_t order_ref_number, char side, uint32_t shares, uint32_t price)
    {
        uint32_t order_index;
        Order *order;
        if (use_indexed_orders)
        {
            order_index = static_cast<uint32_t>(order_ref_number);
            order = &indexed_orders[order_ref_number];
        }
        else
        {
            order_index = order_pool.allocate_index();
            order = order_pool.get(order_index);
            order_indices[order_ref_number] = order_index;
        }
        order->side = side;
        order->shares = shares;

        order->next = MemoryPool<Order>::NIL;
        order->prev = MemoryPool<Order>::NIL;

        absl::flat_hash_map<uint32_t, uint32_t> &price_level_map = price_level_maps[locate];
        uint32_t level_index;
        PriceLevel *level;
        auto [it, inserted] = price_level_map.try_emplace(price, MemoryPool<PriceLevel>::NIL);
        if (inserted)
        {
            level_index = price_level_pool.allocate_index();
            it->second = level_index;
            level = price_level_pool.get(level_index);
            level->price = price;
            level->total_volume = shares;
        }
        else
        {
            level_index = it->second;
            level = price_level_pool.get(level_index);
            level->total_volume += shares;
        }

        order->level = level_index;

        if (level->head == MemoryPool<Order>::NIL)
        {
            level->head = order_index;
            level->tail = order_index;
        }
        else
        {
            if (use_indexed_orders)
                indexed_orders[level->tail].next = order_index;
            else
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
        uint32_t order_index = MemoryPool<Order>::NIL;
        Order *order;
        if (use_indexed_orders)
        {
            order = &indexed_orders[order_ref_number];
            if (order->level == MemoryPool<PriceLevel>::NIL)
                return;
        }
        else
        {
            order_index = order_indices[order_ref_number];
            if (order_index == MemoryPool<Order>::NIL)
                return;
            order = order_pool.get(order_index);
        }

        uint32_t level_index = order->level;
        PriceLevel *level = price_level_pool.get(level_index);

        if (order->next != MemoryPool<Order>::NIL)
        {
            if (use_indexed_orders)
                indexed_orders[order->next].prev = order->prev;
            else
                order_pool.get(order->next)->prev = order->prev;
        }
        else
            level->tail = order->prev;
        if (order->prev != MemoryPool<Order>::NIL)
        {
            if (use_indexed_orders)
                indexed_orders[order->prev].next = order->next;
            else
                order_pool.get(order->prev)->next = order->next;
        }
        else
            level->head = order->next;

        level->total_volume -= order->shares;
        if (recycle_empty_levels_ && level->head == MemoryPool<Order>::NIL)
        {
            price_level_maps[locate].erase(level->price);
            *level = PriceLevel{};
            price_level_pool.deallocate_index(level_index);
        }

        if (use_indexed_orders)
            order->level = MemoryPool<PriceLevel>::NIL;
        else
        {
            order_indices[order_ref_number] = MemoryPool<Order>::NIL;
            order_pool.deallocate_index(order_index);
        }
    }

    void LimitOrderBook::ReplaceOrder(uint16_t locate, uint64_t old_order_ref_number, uint64_t new_order_ref_number, uint32_t shares, uint32_t new_price)
    {
        Order *order;
        if (use_indexed_orders)
        {
            order = &indexed_orders[old_order_ref_number];
            if (order->level == MemoryPool<PriceLevel>::NIL)
                return;
        }
        else
        {
            uint32_t order_index = order_indices[old_order_ref_number];
            if (order_index == MemoryPool<Order>::NIL)
                return;
            order = order_pool.get(order_index);
        }

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
