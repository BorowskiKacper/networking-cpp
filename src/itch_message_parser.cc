

#include <vector>
#include <iostream>

namespace fh_lob
{
    void ParseAddOrder(std::vector<char> &msg_buffer)
    {
        std::cout << "Recognized " << msg_buffer[0] << '\n';
        }

    void ParseMessage(std::vector<char> &msg_buffer)
    {
        switch (msg_buffer[0])
        {
        case 'A':
            ParseAddOrder(msg_buffer);
            break;
        default:
            // std::cout << "Unrecognized code: " << msg_buffer[0] << '\n';
            {
            }
        }
    }
}
