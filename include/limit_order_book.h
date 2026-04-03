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

        Order *allocate();
        void deallocate(Order *order);
    };

    class LimitOrderBook
    {
    private:
        OrderPool order_pool;

        std::unordered_map<uint64_t, Order *> order_map;
        std::unordered_map<uint64_t, PriceLevel *> price_map;

    public:
        LimitOrderBook(size_t max_orders) : order_pool(max_orders) {}

        void add_order(uint64_t id, uint64_t price, uint32_t size, bool is_buy);
        void cancel_order(uint64_t id);
        void execute_order(uint64_t id, uint32_t exec_size);
    };
}