// #include <unistd.h>

#include <iostream>
// #include <iomanip>
// #include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <cstring>
#include <cassert>

namespace fh_lob
{
    void SystemEventMessage(char *msg_buffer, size_t size)
    {
        assert(msg_buffer[0] == 'S' && size == 12);

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

    void StockDirectoryMessage(char *msg_buffer, size_t size)
    {
        assert(msg_buffer[0] == 'R' && size == 39);

        // std::cout <<
    }

    void ParseAddOrder(char *msg_buffer, size_t size)
    {
        // std::cout << "Recognized " << msg_buffer[0] << '\n';
    }

    void ParseMessage(char *msg_buffer, size_t size)
    {
        switch (msg_buffer[0])
        {
        case 'S':
            SystemEventMessage(msg_buffer, size);
            break;
        case 'R':
            StockDirectoryMessage(msg_buffer, size);
            break;
        case 'A':
            // ParseAddOrder(msg_buffer, size);
            // std::cout << "A\n";
            break;
        default:
            // std::cout << "Unrecognized code: " << msg_buffer[0] << '\n';
            {
            }
        }
    }

    void ParseMoldUDP64(char *msg_buffer)
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
            ParseMessage(msg_buffer + i + 2, message_length);
        }
    }
}
