#pragma once

#include "absl/container/flat_hash_map.h"

#include <cstdint>
#include <vector>

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

    struct PriceLevel;

    struct Order
    {
        uint32_t next = MemoryPool<Order>::NIL;
        uint32_t prev = MemoryPool<Order>::NIL;
        uint32_t level = MemoryPool<PriceLevel>::NIL; // back pointer
        uint32_t shares;
        char side;
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

        std::vector<uint32_t> orders;                                          // stock locate --> order
        std::vector<absl::flat_hash_map<uint32_t, uint32_t>> price_level_maps; // stock locate --> price level map | price --> price level index --> price level

        absl::flat_hash_map<uint64_t, uint16_t> str_to_locate;
        std::vector<uint64_t> locate_to_str;

        void ReduceOrderSize(uint64_t id, uint32_t shares);

    public:
        // Pre-allocates every buffer the book will ever use; nothing here grows on demand except price_level_maps/locate_to_str (see MapStockStr). All arguments are capacities, not objects -- note that the first two parameters shadow the members they initialize.
        //
        // order_pool_capacity:
        //     Max number of *live* orders at any instant. A slot is taken by AddOrder and returned by DeleteOrder (and by ReduceOrderSize once shares hit 0), so this only needs to cover peak concurrent open orders, not the day's total. Overflow throws std::runtime_error("pool exhausted") from MemoryPool::allocate_index.
        //     Cost: capacity * (sizeof(Order) == 20 B) + 4 B free-list entry.
        //
        // price_level_pool_capacity:
        //     Max number of distinct (locate, price) pairs seen over the *entire session*. Levels are currently never recycled: the deallocate_index / map-erase in DeleteOrder is commented out, so an emptied level keeps its slot and its map entry forever. Size this for cumulative distinct prices across all symbols, not for concurrent levels.
        //     Cost: capacity * (sizeof(PriceLevel) == 16 B) + 4 B free-list entry.
        //
        // max_locates:
        //     Expected number of ITCH stock locate codes; sizes price_level_maps and locate_to_str, both indexed directly by locate. MapStockStr doubles them (with a warning on stdout) if a larger locate arrives, but AddOrder and ReplaceOrder index price_level_maps[locate] with no bounds check -- so a locate must have been seen by MapStockStr (i.e. its Stock Directory message parsed) before any order message references it.
        //
        // max_order_ref_number:
        //     One past the largest order reference number the feed will use. `orders` is a flat direct-lookup table of this size, indexed by order_ref_number with no bounds check, so anything >= this value is an out-of-bounds read/write. ITCH refs are 64-bit and increase monotonically through the day, so this is effectively "message volume for the session, rounded up".
        //     Cost: 4 B per entry, zero-initialized up front.
        //
        // expected_levels_per_locate:
        //     Reserve hint for each per-locate price level map. Reserving pre-commits the backing arrays so that the typical symbol doesn't rehash; a symbol exceeding the hint still grows normally, just later and less often.
        //     Cost: ~9 B per slot (8 B key+value, 1 B control), slots rounded up to a power of two >= hint/0.875, paid per locate. Default 256 -> ~4.6 KB per map, ~46 MB at 10'000 locates.
        //
        // Both pool capacities are narrowed to uint32_t by MemoryPool's constructor.
        //
        // Example: LimitOrderBook lob(2'000'000, 500 * 100 * 10'000, 10'000, 300'000'000);
        LimitOrderBook(size_t order_pool_capacity, size_t price_level_pool_capacity, size_t max_locates, size_t max_order_ref_number, size_t expected_levels_per_locate = 256) : order_pool(order_pool_capacity), price_level_pool(price_level_pool_capacity)
        {
            price_level_maps.resize(max_locates);
            for (absl::flat_hash_map<uint32_t, uint32_t> &price_level_map : price_level_maps)
                price_level_map.reserve(expected_levels_per_locate);
            locate_to_str.resize(max_locates);
            str_to_locate.reserve(max_locates);
            orders.resize(max_order_ref_number, MemoryPool<Order>::NIL);
        }

        void AddOrder(uint16_t locate, uint64_t order_ref_number, char side, uint32_t shares, uint32_t price);
        void ExecuteOrder(uint64_t order_ref_number, uint32_t shares);
        void ExecuteOrderWithPrice(uint64_t order_ref_number, uint32_t shares, uint32_t price);
        void CancelOrder(uint64_t order_ref_number, uint32_t shares);
        void DeleteOrder(uint64_t order_ref_number);
        void ReplaceOrder(uint16_t locate, uint64_t old_order_ref_number, uint64_t new_order_ref_number, uint32_t shares, uint32_t new_price);

        void MapStockStr(uint64_t stock, uint16_t locate);
    };
}