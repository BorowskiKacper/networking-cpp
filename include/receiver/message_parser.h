#pragma once

#include <vector>
#include <unordered_map>

#include "limit_order_book.h"

namespace fh_lob
{
#pragma pack(push, 1) // Force the compiler to use 1-byte alignment (no-padding)
    struct MsgHeader
    {
        char message_type;
        uint16_t stock_locate;
        uint16_t tracking_number;
        char timestamp[6];
    };

    struct SystemEventMsg
    {
        MsgHeader header;
        char event_code;
    };

    struct StockDirectoryMsg
    {
        MsgHeader header;
        char stock[8];
        char market_category;
        char financial_status_indicator;
        uint32_t round_lot_size;
        char round_lots_only;
        char issue_classification;
        char issue_sub_type[2];
        char authenticity;
        char short_sale_threshold_indicator;
        char ipo_flag;
        char luldreference_price_tier;
        char etp_flag;
        uint32_t etp_leverage_factor;
        char inverse_indicator;
    };

    struct AddOrderMsg
    {
        MsgHeader header;
        uint64_t order_ref_number;
        char buy_sell_indicator;
        uint32_t shares;
        char stock[8];
        uint32_t price;
    };

    struct AddOrderMPIDMsg
    {
        MsgHeader header;
        uint64_t order_ref_number;
        char buy_sell_indicator;
        uint32_t shares;
        char stock[8];
        uint32_t price;
        char attribution[4];
    };

    struct OrderExecutedMsg
    {
        MsgHeader header;
        uint64_t order_ref_number;
        uint32_t executed_shares;
        uint64_t match_number;
    };

    struct OrderExecutedPriceMsg
    {
        MsgHeader header;
        uint64_t order_ref_number;
        uint32_t executed_shares;
        uint64_t match_number;
        char printable;
        uint32_t execution_price;
    };

    struct OrderCancelMsg
    {
        MsgHeader header;
        uint64_t order_ref_number;
        uint32_t cancelled_shares;
    };

    struct OrderDeleteMsg
    {
        MsgHeader header;
        uint64_t order_ref_number;
    };

    struct OrderReplaceMsg
    {
        MsgHeader header;
        uint64_t og_order_ref_number;
        uint64_t new_order_ref_number;
        uint32_t shares;
        uint32_t price;
    };

#pragma pack(pop) // Restore default alignment

    void ParseAddOrder(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    void ParseAddMPIDOrder(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    void ParseOrderExecuted(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    void ParseOrderExecutedPrice(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    void ParseOrderCancel(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    void ParseOrderDelete(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    void ParseOrderReplace(const char *msg_buffer, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);

    bool ParseMessage(const char *msg_buffer, std::unordered_map<std::string, uint16_t> &locate_map, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map);
    bool ParseMoldUDP64(const char *msg_buffer, std::unordered_map<std::string, uint16_t> &locate_map, std::unordered_map<uint16_t, LimitOrderBook *> &lob_map, size_t &total_message_count);
}
