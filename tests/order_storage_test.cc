#include "shared/limit_order_book.h"
#include "shared/message_parser.h"
#include <arpa/inet.h>
#include <cassert>
#include <initializer_list>

namespace fh_lob {
struct OrderStorageTest {
    static void CheckRetainedLevel(LimitOrderBook &b, uint16_t locate, uint32_t price) {
        assert(b.price_level_maps[locate].size() == 1);
        auto *level = b.price_level_pool.get(b.price_level_maps[locate].at(price));
        assert(level->price == price && level->total_volume == 0);
        assert(level->head == UINT32_MAX && level->tail == UINT32_MAX);
    }

    static void Retention(bool indexed) {
        // Omit the new constructor argument to verify the default retains levels.
        LimitOrderBook b(2, 1, 2, 10, 4, indexed);
        uint32_t original_index = UINT32_MAX;
        for (uint32_t i = 0; i < 10; ++i) {
            b.AddOrder(1, 1, 'B', 10, 100);
            if (i == 0) original_index = b.price_level_maps[1].at(100);
            assert(b.price_level_maps[1].at(100) == original_index);
            CheckSingleLevel(b, 1, 100, 10);
            switch (i % 5) {
            case 0: b.DeleteOrder(1, 1); break;
            case 1: b.CancelOrder(1, 1, 10); break;
            case 2: b.ExecuteOrder(1, 1, 10); break;
            case 3: b.ExecuteOrderWithPrice(1, 1, 10, 101); break;
            case 4:
                b.ReplaceOrder(1, 1, 2, 7, 100);
                CheckSingleLevel(b, 1, 100, 7);
                b.DeleteOrder(1, 2);
                break;
            }
            CheckRetainedLevel(b, 1, 100);
            assert(b.price_level_maps[1].at(100) == original_index);
            b.DeleteOrder(1, 1);
        }
        bool exhausted = false;
        try { b.AddOrder(1, 3, 'B', 1, 101); }
        catch (const std::runtime_error &) { exhausted = true; }
        assert(exhausted);
    }

    static void CheckSingleLevel(LimitOrderBook &b, uint16_t locate,
                                 uint32_t price, uint32_t shares) {
        assert(b.price_level_maps[locate].size() == 1);
        auto *level = b.price_level_pool.get(b.price_level_maps[locate].at(price));
        assert(level->price == price && level->total_volume == shares);
        assert(level->head != UINT32_MAX && level->head == level->tail);
        auto *order = b.use_indexed_orders ? &b.indexed_orders[level->head]
                                          : b.order_pool.get(level->head);
        assert(order->next == UINT32_MAX && order->prev == UINT32_MAX);
        assert(order->shares == shares);
        assert(order->level == b.price_level_maps[locate].at(price));
    }

    static void Recycling(bool indexed) {
        LimitOrderBook b(2, 1, 2, 100, 4, indexed, true);
        for (uint32_t i = 0; i < 20; ++i) {
            uint16_t locate = i % 2;
            uint32_t price = 100 + i;
            b.AddOrder(locate, 1, 'B', 10, price);
            CheckSingleLevel(b, locate, price, 10);
            switch (i % 5) {
            case 0: b.DeleteOrder(locate, 1); break;
            case 1: b.CancelOrder(locate, 1, 10); break;
            case 2: b.ExecuteOrder(locate, 1, 10); break;
            case 3: b.ExecuteOrderWithPrice(locate, 1, 10, price + 1); break;
            case 4:
                b.ReplaceOrder(locate, 1, 2, 7, price + 1);
                assert(!b.price_level_maps[locate].contains(price));
                CheckSingleLevel(b, locate, price + 1, 7);
                b.DeleteOrder(locate, 2);
                break;
            }
            assert(b.price_level_maps[0].empty() && b.price_level_maps[1].empty());
            b.DeleteOrder(locate, 1);
        }

        // Recreating the same price must initialize its list and volume afresh.
        b.AddOrder(1, 1, 'S', 3, 100);
        CheckSingleLevel(b, 1, 100, 3);
        b.DeleteOrder(1, 1);
        b.AddOrder(1, 2, 'B', 6, 100);
        CheckSingleLevel(b, 1, 100, 6);

        LimitOrderBook separate(2, 2, 2, 10, 4, indexed, true);
        separate.AddOrder(0, 1, 'B', 4, 100);
        separate.AddOrder(1, 2, 'B', 8, 100);
        separate.DeleteOrder(1, 2);
        assert(separate.price_level_maps[1].empty());
        CheckSingleLevel(separate, 0, 100, 4);
        separate.AddOrder(1, 3, 'S', 2, 101);
        CheckSingleLevel(separate, 1, 101, 2);
        CheckSingleLevel(separate, 0, 100, 4);
    }

    static void ParserRemoval(bool indexed, bool recycle) {
        LimitOrderBook b(2, 1, 2, 10, 4, indexed, recycle);
        auto parse = [&](auto &msg, char type) {
            msg.header.message_type = type;
            msg.header.stock_locate = htons(1);
            msg.order_ref_number = htobe64(1);
            b.AddOrder(1, 1, 'B', 10, 100);
            ParseMessage(reinterpret_cast<const char *>(&msg), b);
            assert(b.price_level_maps[0].empty());
            if (recycle) assert(b.price_level_maps[1].empty());
            else CheckRetainedLevel(b, 1, 100);
        };
        OrderExecutedMsg executed{};
        executed.executed_shares = htonl(10);
        parse(executed, 'E');
        OrderExecutedPriceMsg priced{};
        priced.executed_shares = htonl(10);
        priced.execution_price = htonl(101);
        parse(priced, 'C');
        OrderCancelMsg cancel{};
        cancel.cancelled_shares = htonl(10);
        parse(cancel, 'X');
        OrderDeleteMsg deleted{};
        parse(deleted, 'D');
        b.AddOrder(1, 2, 'S', 5, 100);
        CheckSingleLevel(b, 1, 100, 5);
    }

    static void Run(bool indexed, bool recycle) {
        LimitOrderBook b(4, 8, 2, 100, 4, indexed, recycle);
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
            if (refs.size() == 0 && recycle) {
                assert(!b.price_level_maps[0].contains(price));
                return;
            }
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
        b.DeleteOrder(0,99); check(99,0); queue(100,{0,50},40);
        b.CancelOrder(0,0,3); check(0,7); queue(100,{0,50},37);
        b.ExecuteOrder(0,0,7); check(0,0); queue(100,{50},30);
        b.ReplaceOrder(0,50,60,40,101); check(50,0); check(60,40);
        queue(100,{},0); queue(101,{60},40);
        b.ExecuteOrderWithPrice(0,60,40,101); check(60,0); queue(101,{},0);
        b.DeleteOrder(0,60); b.CancelOrder(0,80,1); b.ReplaceOrder(0,80,81,2,100);
        check(81,0);
        b.AddOrder(0,1,'S',5,100); queue(100,{1},5);
        b.AddOrder(0,2,'S',5,100); b.DeleteOrder(0,2); queue(100,{1},5);
        if (indexed) assert(b.order_indices.empty());
        else assert(b.indexed_orders.empty());

        LimitOrderBook small(1, 4, 2, 10, 4, indexed, recycle);
        small.AddOrder(0,1,'B',1,100);
        bool exhausted = false;
        try { small.AddOrder(0,2,'B',1,100); }
        catch (const std::runtime_error &) { exhausted = true; }
        assert(exhausted == !indexed);
        small.DeleteOrder(0,1);
        small.AddOrder(0,3,'B',1,100);
    }
};
}
int main() {
    for (bool indexed : {false, true}) {
        fh_lob::OrderStorageTest::Recycling(indexed);
        fh_lob::OrderStorageTest::Retention(indexed);
        for (bool recycle : {false, true}) {
            fh_lob::OrderStorageTest::Run(indexed, recycle);
            fh_lob::OrderStorageTest::ParserRemoval(indexed, recycle);
        }
    }
    bool rejected = false;
    try { fh_lob::LimitOrderBook b(0,0,0,static_cast<uint64_t>(UINT32_MAX)+1,0,true); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
}
