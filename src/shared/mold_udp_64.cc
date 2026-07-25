#include "shared/mold_udp_64.h"

#include <arpa/inet.h>
#include <endian.h>

#include <algorithm>
#include <cstring>

namespace fh_lob
{
    MoldUDP64Builder::MoldUDP64Builder(std::string_view session, uint64_t first_sequence)
        : sequence_number_(first_sequence)
    {
        // Session field is alphanumeric, right-padded with spaces (MoldUDP64 spec)
        memset(buffer_, ' ', sizeof(MoldUDP64Header::session));
        memcpy(buffer_, session.data(), std::min(session.size(), sizeof(MoldUDP64Header::session)));
    }

    bool MoldUDP64Builder::TryAppend(const char *payload, uint16_t length)
    {
        if (write_index_ + sizeof(uint16_t) + length > k_max_packet_size) [[unlikely]] return false;

        const uint16_t net_length = htons(length);
        memcpy(buffer_ + write_index_, &net_length, sizeof(net_length));
        memcpy(buffer_ + write_index_ + sizeof(net_length), payload, length);
        write_index_ += sizeof(net_length) + length;
        message_count_++;
        return true;
    }

    bool MoldUDP64Builder::Empty() const
    {
        return message_count_ == 0;
    }

    const char *MoldUDP64Builder::Finalize()
    {
        const uint64_t net_sequence = htobe64(sequence_number_);
        const uint16_t net_count = htons(message_count_);
        memcpy(buffer_ + offsetof(MoldUDP64Header, sequence_number), &net_sequence, sizeof(net_sequence));
        memcpy(buffer_ + offsetof(MoldUDP64Header, message_count), &net_count, sizeof(net_count));
        return buffer_;
    }

    const char *MoldUDP64Builder::FinalizeEndOfSession()
    {
        // Carries the sequence number past the block just sent and clears the
        // pending payload, leaving write_index_ at the bare header. On repeat
        // calls message_count_ is already 0, so the packet stays byte-identical.
        Reset();

        const uint64_t net_sequence = htobe64(sequence_number_);
        const uint16_t net_count = htons(k_end_of_session_count);
        memcpy(buffer_ + offsetof(MoldUDP64Header, sequence_number), &net_sequence, sizeof(net_sequence));
        memcpy(buffer_ + offsetof(MoldUDP64Header, message_count), &net_count, sizeof(net_count));
        return buffer_;
    }

    size_t MoldUDP64Builder::size() const
    {
        return write_index_;
    }

    void MoldUDP64Builder::Reset()
    {
        sequence_number_ += message_count_;
        message_count_ = 0;
        write_index_ = sizeof(MoldUDP64Header);
    }


    MoldUDP64View::MoldUDP64View(const char *packet, size_t length)
        : packet_(packet), packet_length_(length)
    {
        uint64_t net_sequence;
        uint16_t net_count;
        memcpy(&net_sequence, packet_ + offsetof(MoldUDP64Header, sequence_number), sizeof(net_sequence));
        memcpy(&net_count, packet_ + offsetof(MoldUDP64Header, message_count), sizeof(net_count));
        sequence_number_ = be64toh(net_sequence);
        message_count_ = ntohs(net_count);
    }

    uint64_t MoldUDP64View::sequence_number() const
    {
        return sequence_number_;
    }

    uint16_t MoldUDP64View::message_count() const
    {
        return message_count_;
    }

    std::string_view MoldUDP64View::session() const
    {
        return {packet_, sizeof(MoldUDP64Header::session)};
    }

    bool MoldUDP64View::is_heartbeat() const
    {
        return message_count_ == 0;
    }

    bool MoldUDP64View::is_end_of_session() const
    {
        return message_count_ == k_end_of_session_count;
    }

    MoldUDP64View::Iterator MoldUDP64View::begin() const
    {
        // 0xFFFF is an End of Session sentinel, not a block count: iterate nothing
        const uint16_t count = is_end_of_session() ? uint16_t{0} : message_count_;
        return {packet_ + sizeof(MoldUDP64Header), count};
    }

    MoldUDP64View::Iterator MoldUDP64View::end() const
    {
        return {nullptr, 0};
    }
}
