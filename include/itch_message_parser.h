#pragma once

#include <vector>

namespace fh_lob
{
    void ParseAddOrder(char *msg_buffer, size_t size);
    void ParseMessage(char *msg_buffer, size_t size);
    void ParseMoldUDP64(char *msg_buffer);
}
