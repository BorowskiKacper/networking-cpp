#include <cstdint>
#include <vector>
#include <unordered_map>

namespace fh_lob
{
    struct Order
    {
        uint64_t id;
        uint64_t price;
        uint32_t size;
        bool is_buy;

        Order *next = nullptr;
        Order *prev = nullptr;
    };

    struct PriceLevel
    {
        uint64_t price;
        uint32_t total_volume;

        Order *head = nullptr;
        Order *tail = nullptr;
    };

    class OrderPool
    {
    private:
        std::vector<Order> pool;
        std::vector<Order *> free_list;

    public:
        OrderPool(size_t capacity);
        {
            pool.resize(capacity);
            free_list.reserve(capacity);
            // Push all pointers to the free list
            for (size_t = 0; i < capacity; i++)
            {
                free_list.append(&pool[i]);
            }
        }

        Order *allocate()
        {
            if (free_list.empty())
                return nullptr;
            Order *order = free_list.back();
            free_list.pop_back();
            return order;
        }

        void deallocate(Order *order)
        {
            order->next = nullptr;
            order->prev = nullptr;
            free_list.push_back(order);
        }
    }

    class LimitOrderBook
    {
    private:
        OrderPool order_pool;

        std::unordered_map<uint64_t, Order *> order_map;
        std::unordered_map<uint64_t, PriceLevel *> price_map;

    public:
        LimitOrderBook(size_t max_orders) : order_pool(max_orders) {}
    }

    // class LOB
    // {
    // public:
    //     LOB();

    // private:
    // }

}