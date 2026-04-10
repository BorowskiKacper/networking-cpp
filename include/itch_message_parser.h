

#include <vector>

namespace fh_lob
{
    void ParseAddOrder(std::vector<char> &msg_buffer);
    void ParseMessage(std::vector<char> &msg_buffer);
    void ParseMoldUDP64(std::vector<char> &msg_buffer);
}
