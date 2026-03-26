#include <iostream>
#include <fstream>
#include <vector>
#include <arpa/inet.h>

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

    while (file.read(reinterpret_cast<char *>(&msg_length), sizeof(msg_length)))
    {
        msg_length = ntohs(msg_length);

        vector<char> msg_buffer(msg_length);
        if (file.read(msg_buffer.data(), msg_length))
        {
            ParseMessage(msg_buffer);
        }
    }

    return EXIT_SUCCESS;
}