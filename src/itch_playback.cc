#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <arpa/inet.h>

void print_buffer(std::vector<char> &msg_buffer, uint16_t msg_length)
{
    if (msg_length > 0)
        std::cout << msg_buffer[0] << " ";
    for (int i = 1; i < msg_length; i++)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (static_cast<unsigned int>(static_cast<unsigned char>(msg_buffer[i]))) << " ";
    }
    std::cout << std::dec;
}

// namespace fh_lob
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <multicast_ip> <port>\n"; // e.g., "239.0.0.1" 12345
        return EXIT_FAILURE;
    }

    const char *multicast_ip = argv[1];
    int port = atoi(argv[2]);

    std::ifstream file("itchmessages/12302019.NASDAQ_ITCH50", std::ios::binary);

    if (!file)
    {
        std::cout << "Failed to open file. " << std::endl;
        return EXIT_FAILURE;
    }

    uint16_t msg_length;

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0)
    {
        perror("Sorry, couldn't create the socket");
        return EXIT_FAILURE;
    }

    unsigned char ttl = 1;
    if (setsockopt(udp_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    {
        perror("Setting ttl failed");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in multicast_addr = {.sin_family = AF_INET, .sin_port = htons(port)};
    if (inet_pton(AF_INET, multicast_ip, &(multicast_addr.sin_addr.s_addr)) <= 0)
    {
        perror("Something went wrong with the IP address");
        return EXIT_FAILURE;
    }

    uint64_t i = 0;

    while (file.read(reinterpret_cast<char *>(&msg_length), sizeof(msg_length)))
    {
        msg_length = ntohs(msg_length);

        std::vector<char> msg_buffer(msg_length);

        file.read(msg_buffer.data(), msg_length);

        if (sendto(udp_fd, msg_buffer.data(), msg_length, 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
        {
            perror("Failed to send message");
            close(udp_fd);
            return EXIT_FAILURE;
        };

        i++;
    }
    close(udp_fd);

    return EXIT_SUCCESS;
}
