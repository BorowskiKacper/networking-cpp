#pragma once

#include <arpa/inet.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace fh_lob
{
#pragma pack(push, 1)
    struct MoldUDP64Header
    {
        char session[10];
        uint64_t sequence_number; // big-endian on the wire
        uint16_t message_count;   // big-endian on the wire
    };
#pragma pack(pop) 
    static_assert(sizeof(MoldUDP64Header) == 20);

    // message_count == 0xFFFF signals End of Session
    inline constexpr uint16_t k_end_of_session_count = 0xFFFF;

    class MoldUDP64Builder
    {
    public:
        static constexpr size_t k_max_packet_size = 1472;

        explicit MoldUDP64Builder(std::string_view session, uint64_t first_sequence = 1);

        // Writes a 2-byte length prefix, then copies the payload.
        // If payload wouldn't fit --> function returns false and 
        // caller should send the Finalize()'d packet, call Reset(), then retry.
        bool TryAppend(const char *payload, uint16_t length);

        bool Empty() const;     // checks if anything is pending
        const char *Finalize(); // stamps seq/count (network order), returns buffer
        size_t size() const;    // bytes to pass to sendto()
        void Reset();

    private:
        char buffer_[k_max_packet_size];
        size_t write_index_ = sizeof(MoldUDP64Header);
        uint64_t sequence_number_;
        uint16_t message_count_ = 0;
    };

    // Non-owning, zero-copy view over a received packet. The packet buffer must
    // outlive the view and any iterators/Messages taken from it.
    class MoldUDP64View
    {
    public:
        MoldUDP64View(const char *packet, size_t length);

        uint64_t sequence_number() const; // be64toh'd
        uint16_t message_count() const;
        std::string_view session() const;
        bool is_heartbeat() const;      // count == 0
        bool is_end_of_session() const; // count == 0xFFFF

        struct Message
        {
            const char *payload;
            uint16_t length;
        };

        class Iterator
        {
        public:
            Iterator(const char *cursor, uint16_t remaining)
                : cursor_(cursor), remaining_(remaining) {}

            Message operator*() const
            {
                return {cursor_ + sizeof(uint16_t), ReadLength()};
            }
            Iterator &operator++()
            {
                cursor_ += sizeof(uint16_t) + ReadLength();
                remaining_--;
                return *this;
            }

            bool operator!=(const Iterator &other) const
            {
                return remaining_ != other.remaining_;
            }

        private:
            uint16_t ReadLength() const
            {
                uint16_t net_length;
                memcpy(&net_length, cursor_, sizeof(net_length));
                return ntohs(net_length);
            }

            const char *cursor_; // points at a 2-byte length prefix
            uint16_t remaining_;
        };
        Iterator begin() const;
        Iterator end() const;

    private:
        const char *packet_;
        size_t packet_length_;
        uint64_t sequence_number_;
        uint16_t message_count_;
    };
}
