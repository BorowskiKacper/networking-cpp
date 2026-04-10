// #include <unistd.h>

#include <iostream>
// #include <iomanip>
// #include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <cstring>

namespace fh_lob
{
    void ParseAddOrder(char *msg_buffer, size_t size)
    {
        std::cout << "Recognized " << msg_buffer[0] << '\n';
    }

    void ParseMessage(char *msg_buffer, size_t size)
    {
        switch (msg_buffer[0])
        {
        case 'A':
            // ParseAddOrder(msg_buffer, size);
            std::cout << "A\n";
            break;
        default:
            std::cout << "Unrecognized code: " << msg_buffer[0] << '\n';
            {
            }
        }
    }

    void ParseMoldUDP64(std::vector<char> &msg_buffer)
    {
        uint64_t sequence_number;
        uint16_t message_count;
        memcpy(&sequence_number, msg_buffer.data() + 10, sizeof(sequence_number));
        memcpy(&message_count, msg_buffer.data() + 18, sizeof(message_count));
        uint64_t sequence_number = be64toh(sequence_number);
        uint16_t message_count = ntohs(message_count);

        uint16_t message_length;
        for (size_t i = 20; message_count > 0; message_count--, i += 2 + message_length)
        {
            memcpy(&message_length, msg_buffer.data() + i, sizeof(message_length));
            message_length = ntohs(message_length);
            ParseMessage(msg_buffer.data() + i + 2, message_length);
        }
    }
}
