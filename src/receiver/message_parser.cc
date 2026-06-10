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
    bool SystemEventMessage(const char *msg_buffer)
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
            return true;
            // print = "End of Messages";
            break;
        }
        std::cout << print << std::endl;
        return false;
    }

    void StockDirectoryMessage(const char *msg_buffer, LimitOrderBook &lob)
    {
        const auto *msg = reinterpret_cast<const StockDirectoryMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        lob.MapStockStr(reinterpret_cast<uint64_t>(msg->stock, sizeof(char) * 8), locate);
    }

    void ParseAddOrder(const char *msg_buffer, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        const auto *msg = reinterpret_cast<const AddOrderMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        if (locate < lob_ptrs.size())
            lob_ptrs[locate]->AddOrder(msg->order_ref_number,
                                       msg->buy_sell_indicator,
                                       msg->shares,
                                       msg->price);
    }

    void ParseOrderExecuted(const char *msg_buffer, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        const auto *msg = reinterpret_cast<const OrderExecutedMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        if (locate < lob_ptrs.size())
            lob_ptrs[locate]->ExecuteOrder(msg->order_ref_number,
                                           msg->executed_shares);
    }
    void ParseOrderExecutedPrice(const char *msg_buffer, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        const auto *msg = reinterpret_cast<const OrderExecutedPriceMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        if (locate < lob_ptrs.size())
            lob_ptrs[locate]->ExecuteOrderWithPrice(msg->order_ref_number,
                                                    msg->executed_shares,
                                                    msg->execution_price);
    }
    void ParseOrderCancel(const char *msg_buffer, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        const auto *msg = reinterpret_cast<const OrderCancelMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        if (locate < lob_ptrs.size())
            lob_ptrs[locate]->CancelOrder(msg->order_ref_number,
                                          msg->cancelled_shares);
    }
    void ParseOrderDelete(const char *msg_buffer, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        const auto *msg = reinterpret_cast<const OrderDeleteMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        if (locate < lob_ptrs.size())
            lob_ptrs[locate]->DeleteOrder(msg->order_ref_number);
    }
    void ParseOrderReplace(const char *msg_buffer, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        const auto *msg = reinterpret_cast<const OrderReplaceMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        if (locate < lob_ptrs.size())
            lob_ptrs[locate]->ReplaceOrder(msg->og_order_ref_number,
                                           msg->new_order_ref_number,
                                           msg->shares,
                                           msg->price);
    }

    bool ParseMessage(const char *msg_buffer, absl::flat_hash_map<std::string, uint16_t> &locate_map, std::vector<LimitOrderBook *> &lob_ptrs)
    {
        switch (msg_buffer[0])
        {
        case 'S':
            if (SystemEventMessage(msg_buffer))
            {
                return true;
            }
            break;
        case 'R':
            StockDirectoryMessage(msg_buffer, locate_map, lob_ptrs);
            break;
        case 'A':
            ParseAddOrder(msg_buffer, lob_ptrs);
            break;
        case 'F':
            ParseAddOrder(msg_buffer, lob_ptrs);
            break;
        case 'E':
            ParseOrderExecuted(msg_buffer, lob_ptrs);
            break;
        case 'C':
            ParseOrderExecutedPrice(msg_buffer, lob_ptrs);
            break;
        case 'X':
            ParseOrderCancel(msg_buffer, lob_ptrs);
            break;
        case 'D':
            ParseOrderDelete(msg_buffer, lob_ptrs);
            break;
        case 'U':
            ParseOrderReplace(msg_buffer, lob_ptrs);
            break;
        default:
        {
        }
            // Ignore non-LOB messages silently
        }

        return false;
    }

    bool ParseMoldUDP64(const char *msg_buffer, absl::flat_hash_map<std::string, uint16_t> &locate_map, std::vector<LimitOrderBook *> &lob_ptrs, size_t &total_message_count)
    {
        uint64_t sequence_number;
        uint16_t message_count;
        memcpy(&sequence_number, msg_buffer + 10, sizeof(sequence_number));
        memcpy(&message_count, msg_buffer + 18, sizeof(message_count));
        sequence_number = be64toh(sequence_number);
        message_count = ntohs(message_count);

        uint16_t message_length;
        for (size_t i = 20; message_count > 0; message_count--, i += 2 + message_length)
        {
            memcpy(&message_length, msg_buffer + i, sizeof(message_length));
            message_length = ntohs(message_length);

            uint8_t msg_type = static_cast<uint8_t>(msg_buffer[i + 2]);
            uint64_t start_cycle = bench::RdtscStart();
            bool is_complete = ParseMessage(msg_buffer + i + 2, locate_map, lob_ptrs);
            uint64_t end_cycle = bench::RdtscEnd();
            if (total_message_count > 10000) // warmup skip
                bench::hist[msg_type].Record(end_cycle - start_cycle);
            total_message_count++;

            if (is_complete)
                return true;
        }

        return false;
    }
}
