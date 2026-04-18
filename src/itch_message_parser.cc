#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <cstring>
#include <cassert>
#include <unordered_map>

#include "../include/itch_message_parser.h"
#include "../include/limit_order_book.h"

namespace fh_lob
{
    void SystemEventMessage(const char *msg_buffer)
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

    void StockDirectoryMessage(const char *msg_buffer, std::unordered_map<std::string, uint16_t> &locate_map, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const StockDirectoryMsg *>(msg_buffer);

        std::string stock_str(msg->stock, sizeof(char) * 8);
        if (locate_map.find(stock_str) == locate_map.end())
            locate_map[stock_str] = msg->header.stock_locate;
        if (lob_map.find(msg->header.stock_locate) == lob_map.end())
            lob_map[msg->header.stock_locate] = new LimitOrderBook(10000);
    }

    void ParseAddOrder(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const AddOrderMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        auto it = lob_map.find(locate);
        if (it == lob_map.end())
            return;
        it->second->AddOrder(msg->order_ref_number,
                             msg->buy_sell_indicator,
                             msg->shares,
                             msg->price);
    }

    void ParseOrderExecuted(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const OrderExecutedMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        auto it = lob_map.find(locate);
        if (it == lob_map.end())
            return;
        it->second->ExecuteOrder(msg->order_ref_number,
                                 msg->executed_shares);
    }
    void ParseOrderExecutedPrice(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const OrderExecutedPriceMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        auto it = lob_map.find(locate);
        if (it == lob_map.end())
            return;
        it->second->ExecuteOrderWithPrice(msg->order_ref_number,
                                          msg->executed_shares,
                                          msg->execution_price);
    }
    void ParseOrderCancel(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const OrderCancelMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        auto it = lob_map.find(locate);
        if (it == lob_map.end())
            return;
        it->second->CancelOrder(msg->order_ref_number,
                                msg->cancelled_shares);
    }
    void ParseOrderDelete(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const OrderDeleteMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        auto it = lob_map.find(locate);
        if (it == lob_map.end())
            return;
        it->second->DeleteOrder(msg->order_ref_number);
    }
    void ParseOrderReplace(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        const auto *msg = reinterpret_cast<const OrderReplaceMsg *>(msg_buffer);
        uint16_t locate = ntohs(msg->header.stock_locate);
        auto it = lob_map.find(locate);
        if (it == lob_map.end())
            return;
        it->second->ReplaceOrder(msg->og_order_ref_number,
                                 msg->new_order_ref_number,
                                 msg->shares,
                                 msg->price);
    }

    void ParseMessage(const char *msg_buffer, std::unordered_map<std::string, uint16_t> &locate_map, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
    {
        switch (msg_buffer[0])
        {
        case 'S':
            SystemEventMessage(msg_buffer);
            break;
        case 'R':
            StockDirectoryMessage(msg_buffer, locate_map, lob_map);
            break;
        case 'A':
            ParseAddOrder(msg_buffer, lob_map);
            break;
        case 'F':
            ParseAddOrder(msg_buffer, lob_map);
            break;
        case 'E':
            ParseOrderExecuted(msg_buffer, lob_map);
            break;
        case 'C':
            ParseOrderExecutedPrice(msg_buffer, lob_map);
            break;
        case 'X':
            ParseOrderCancel(msg_buffer, lob_map);
            break;
        case 'D':
            ParseOrderDelete(msg_buffer, lob_map);
            break;
        case 'U':
            ParseOrderReplace(msg_buffer, lob_map);
            break;
        default:
        {
        }
            // Ignore non-LOB messages silently
        }
    }

    void ParseMoldUDP64(const char *msg_buffer, std::unordered_map<std::string, uint16_t> &locate_map, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map)
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
            ParseMessage(msg_buffer + i + 2, locate_map, lob_map);
        }
    }
}
