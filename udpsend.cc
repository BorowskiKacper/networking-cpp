#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <peer_ip> <peer_port> <message>\n";
        return EXIT_FAILURE;
    }

    // get some info about our peer from out args
    const char *peer_ip = argv[1];
    int peer_port = atoi(argv[2]);
    const char *message = argv[3];

    struct sockaddr_in peer_addr = {.sin_family = AF_INET, .sin_port = htons(peer_port)};

    if (inet_pton(AF_INET, peer_ip, &(peer_addr.sin_addr)) <= 0)
    {
        perror("Something went wrong with the IP address");
        return EXIT_FAILURE;
    }

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0)
    {
        perror("Sorry, couldn't create the socket");
        return EXIT_FAILURE;
    }

    if (sendto(udp_fd, message, strlen(message) + 1, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0)
    {
        perror("Failed to send message");
        close(udp_fd);
        return EXIT_FAILURE;
    };

    std::cout << "Sent \"" << message << "\" to " << peer_ip << ":" << peer_port << '\n';
    close(udp_fd);

    return EXIT_SUCCESS;
}