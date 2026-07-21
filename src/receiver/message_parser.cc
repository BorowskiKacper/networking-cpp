#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <cstring>
#include <cassert>
#include "absl/container/flat_hash_map.h"

#include "receiver/message_parser.h"
#include "receiver/limit_order_book.h"
#include "receiver/benchmark.h"

namespace fh_lob
{
    void ParseSystemEvent(const char *msg_buffer)
    {
        std::string print;
        switch (msg_buffer[11])
        {
        case 'O':
            print = "Start of Messages";
            break;
        case 'S':
            print = "Start of System Hours";
            break;
        case 'Q':
            print = "Start of Market Hours";
            break;
        case 'M':
            print = "End of Market Hours";
            break;
        case 'E':
            print = "End of System Hours";
            break;
        case 'C':
            print = "End of Messages";
            break;
        }
        std::cout << print << std::endl;
    }

    void ParseStockDirectory(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const StockDirectoryMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        uint64_t stock;
        std::memcpy(&stock, msg->stock, sizeof(stock));
        lob.MapStockStr(stock, locate);
    }

    void ParseAddOrder(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const AddOrderMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        uint64_t order_ref_number = be64toh(msg->order_ref_number);
        uint32_t shares = ntohl(msg->shares);
        uint32_t price = ntohl(msg->price);
        lob.AddOrder(locate,
                     order_ref_number,
                     msg->buy_sell_indicator,
                     shares,
                     price);
    }

    void ParseOrderExecuted(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const OrderExecutedMsg *>(msg_buffer);
        uint64_t order_ref_number = be64toh(msg->order_ref_number);
        uint32_t executed_shares = ntohl(msg->executed_shares);
        lob.ExecuteOrder(order_ref_number,
                         executed_shares);
    }
    void ParseOrderExecutedPrice(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const OrderExecutedPriceMsg *>(msg_buffer);
        uint64_t order_ref_number = be64toh(msg->order_ref_number);
        uint32_t executed_shares = ntohl(msg->executed_shares);
        uint32_t execution_price = ntohl(msg->execution_price);
        lob.ExecuteOrderWithPrice(order_ref_number,
                                  executed_shares,
                                  execution_price);
    }
    void ParseOrderCancel(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const OrderCancelMsg *>(msg_buffer);
        uint64_t order_ref_number = be64toh(msg->order_ref_number);
        uint32_t cancelled_shares = ntohl(msg->cancelled_shares);
        lob.CancelOrder(order_ref_number,
                        cancelled_shares);
    }
    void ParseOrderDelete(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const OrderDeleteMsg *>(msg_buffer);
        uint64_t order_ref_number = be64toh(msg->order_ref_number);
        lob.DeleteOrder(order_ref_number);
    }
    void ParseOrderReplace(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const OrderReplaceMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        uint64_t og_order_ref_number = be64toh(msg->og_order_ref_number);
        uint64_t new_order_ref_number = be64toh(msg->new_order_ref_number);
        uint32_t shares = ntohl(msg->shares);
        uint32_t price = ntohl(msg->price);
        lob.ReplaceOrder(locate,
                         og_order_ref_number,
                         new_order_ref_number,
                         shares,
                         price);
    }

    void ParseMessage(const char *msg_buffer, LimitOrderBook &lob)
    {
        switch (msg_buffer[0])
        {
        case 'S':
            ParseSystemEvent(msg_buffer);
            break;
        case 'R':
            ParseStockDirectory(msg_buffer, lob);
            break;
        case 'A':
            ParseAddOrder(msg_buffer, lob);
            break;
        case 'F':
            ParseAddOrder(msg_buffer, lob);
            break;
        case 'E':
            ParseOrderExecuted(msg_buffer, lob);
            break;
        case 'C':
            ParseOrderExecutedPrice(msg_buffer, lob);
            break;
        case 'X':
            ParseOrderCancel(msg_buffer, lob);
            break;
        case 'D':
            ParseOrderDelete(msg_buffer, lob);
            break;
        case 'U':
            ParseOrderReplace(msg_buffer, lob);
            break;
        default:
        {
        }
            // Ignore non-LOB messages silently
        }
    }

    // bool ParseMoldUDP64(const char *msg_buffer, LimitOrderBook &lob, size_t &total_message_count)
    // {
    //     uint64_t sequence_number;
    //     uint16_t message_count;
    //     memcpy(&sequence_number, msg_buffer + 10, sizeof(sequence_number));
    //     memcpy(&message_count, msg_buffer + 18, sizeof(message_count));
    //     sequence_number = be64toh(sequence_number);
    //     message_count = ntohs(message_count);

    //     uint16_t message_length;
    //     for (size_t i = 20; message_count > 0; message_count--, i += 2 + message_length)
    //     {
    //         memcpy(&message_length, msg_buffer + i, sizeof(message_length));
    //         message_length = ntohs(message_length);

    //         uint8_t msg_type = static_cast<uint8_t>(msg_buffer[i + 2]);
    //         uint64_t start_cycle = bench::RdtscStart();
    //         bool is_complete = ParseMessage(msg_buffer + i + 2, lob);
    //         uint64_t end_cycle = bench::RdtscEnd();
    //         if (total_message_count > 10000) // warmup skip
    //             bench::hist[msg_type].Record(end_cycle - start_cycle);
    //         total_message_count++;

    //         if (is_complete)
    //             return true;
    //     }

    //     return false;
    // }
}
