#include "shared/limit_order_book.h"
#include <cassert>
#include <initializer_list>

namespace fh_lob {
struct OrderStorageTest {
    static void Run(bool indexed) {
        LimitOrderBook b(4, 8, 2, 100, 4, indexed);
        auto get = [&](uint64_t ref) -> Order * {
            if (indexed) return b.indexed_orders[ref].level == UINT32_MAX ? nullptr : &b.indexed_orders[ref];
            return b.order_indices[ref] == UINT32_MAX ? nullptr : b.order_pool.get(b.order_indices[ref]);
        };
        auto check = [&](uint64_t ref, uint32_t shares) {
            auto *order = get(ref);
            if (!shares) assert(order == nullptr);
            else { assert(order); assert(order->shares == shares); }
        };
        auto queue = [&](uint32_t price, std::initializer_list<uint64_t> refs, uint32_t volume) {
            auto *level = b.price_level_pool.get(b.price_level_maps[0].at(price));
            assert(level->total_volume == volume);
            uint32_t id = level->head, previous = UINT32_MAX;
            for (auto ref : refs) {
                assert(id == (indexed ? ref : b.order_indices[ref]));
                auto *order = get(ref);
                assert(order && order->prev == previous);
                previous = id;
                id = order->next;
            }
            assert(id == UINT32_MAX && level->tail == previous);
        };
        b.AddOrder(0, 0, 'B', 10, 100);
        b.AddOrder(0, 99, 'B', 20, 100);
        b.AddOrder(0, 50, 'B', 30, 100);
        queue(100, {0,99,50}, 60);
        b.DeleteOrder(99); check(99,0); queue(100,{0,50},40);
        b.CancelOrder(0,3); check(0,7); queue(100,{0,50},37);
        b.ExecuteOrder(0,7); check(0,0); queue(100,{50},30);
        b.ReplaceOrder(0,50,60,40,101); check(50,0); check(60,40);
        queue(100,{},0); queue(101,{60},40);
        b.ExecuteOrderWithPrice(60,40,101); check(60,0); queue(101,{},0);
        b.DeleteOrder(60); b.CancelOrder(80,1); b.ReplaceOrder(0,80,81,2,100);
        check(81,0);
        b.AddOrder(0,1,'S',5,100); queue(100,{1},5);
        b.AddOrder(0,2,'S',5,100); b.DeleteOrder(2); queue(100,{1},5);
        if (indexed) assert(b.order_indices.empty());
        else assert(b.indexed_orders.empty());

        LimitOrderBook small(1, 4, 2, 10, 4, indexed);
        small.AddOrder(0,1,'B',1,100);
        bool exhausted = false;
        try { small.AddOrder(0,2,'B',1,100); }
        catch (const std::runtime_error &) { exhausted = true; }
        assert(exhausted == !indexed);
        small.DeleteOrder(1);
        small.AddOrder(0,3,'B',1,100);
    }
};
}
int main() {
    fh_lob::OrderStorageTest::Run(false);
    fh_lob::OrderStorageTest::Run(true);
    bool rejected = false;
    try { fh_lob::LimitOrderBook b(0,0,0,static_cast<uint64_t>(UINT32_MAX)+1,0,true); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
}
