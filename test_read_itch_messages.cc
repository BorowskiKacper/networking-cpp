#include <iostream>
#include <fstream>
#include <vector>
#include <arpa/inet.h>

#include "include/itch_message_parser.h"

using std::vector;

int main()
{

    std::ifstream file("itchmessages/12302019.NASDAQ_ITCH50", std::ios::binary);

    if (!file)
    {
        std::cout << "Failed to open file. " << std::endl;
        return EXIT_FAILURE;
    }

    uint16_t msg_length;

    uint64_t i = 0;

    while (file.read(reinterpret_cast<char *>(&msg_length), sizeof(msg_length)) && i < 500)
    {
        msg_length = ntohs(msg_length);

        vector<char> msg_buffer(msg_length);
        if (file.read(msg_buffer.data(), msg_length))
        {
            fh_lob::ParseMessage(msg_buffer);
        }

        i++;
    }

    return EXIT_SUCCESS;
}