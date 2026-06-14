#include <cstdint>
#include <vector>
#include "absl/container/flat_hash_map.h"
#include <iostream>

#include "receiver/limit_order_book.h"

namespace fh_lob
{
    void LimitOrderBook::ReduceOrderSize(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        Order *order;
        try
        {
            order = orders[order_ref_number];
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }

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
        Order *order = order_pool.allocate();
        order->order_ref_number = order_ref_number;
        order->side = side;
        order->shares = shares;
        order->price = price;

        order->next = nullptr;
        order->prev = nullptr;

        try
        {
            orders[order_ref_number] = order;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }

        absl::flat_hash_map<uint32_t, PriceLevel *> &price_level_map = price_level_maps[locate];
        auto [it, inserted] = price_level_map.try_emplace(price, nullptr);
        if (inserted)
        {
            it->second = price_level_pool.allocate();
            it->second->price = price;
            it->second->total_volume = 0;
        }

        PriceLevel *level = it->second;
        level->total_volume += shares;

        if (level->head == nullptr)
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

    void LimitOrderBook::ExecuteOrder(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        ReduceOrderSize(locate, order_ref_number, shares);
    }

    void LimitOrderBook::ExecuteOrderWithPrice(uint16_t locate, uint64_t order_ref_number, uint32_t shares, uint32_t price)
    {
        ReduceOrderSize(locate, order_ref_number, shares);
        price = price; // handle warning
    }

    void LimitOrderBook::CancelOrder(uint16_t locate, uint64_t order_ref_number, uint32_t shares)
    {
        ReduceOrderSize(locate, order_ref_number, shares);
    }

    void LimitOrderBook::DeleteOrder(uint16_t locate, uint64_t order_ref_number)
    {
        Order *order;
        try
        {
            order = orders[order_ref_number];
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }

        absl::flat_hash_map<uint32_t, PriceLevel *> &price_level_map = price_level_maps[locate];
        PriceLevel *level = price_level_map[order->price];

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
            // price_level_map.erase(order->price);
        }

        // order_map.erase(id);
        orders[order_ref_number] = nullptr;
        order_pool.deallocate(order);
    }

    void LimitOrderBook::ReplaceOrder(uint16_t locate, uint64_t old_order_ref_number, uint64_t new_order_ref_number, uint32_t shares, uint32_t new_price)
    {
        Order *order;
        try
        {
            order = orders[old_order_ref_number];
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
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