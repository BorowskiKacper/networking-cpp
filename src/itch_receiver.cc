#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
// #include <cstdlib>
#include <cstring>

#define BUFFER_SIZE 1024

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <multicast_ip> <port>\n"; // e.g., "239.0.0.1" 12345
        return EXIT_FAILURE;
    }

    const char *multicast_ip = argv[1];
    int port = atoi(argv[2]);

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0)
    {
        perror("Sorry, couldn't create the socket");
        return EXIT_FAILURE;
    }

    int reuse = 1;
    if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
    {
        perror("Setting SO_REUSEADDR failed");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in local_addr =
        {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {.s_addr = INADDR_ANY}};

    if (bind(udp_fd, (sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        perror("Binding to socket failed");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    struct ip_mreq multicast_req = {.imr_interface = {.s_addr = INADDR_ANY}};
    if (inet_pton(AF_INET, multicast_ip, &(multicast_req.imr_multiaddr.s_addr)) <= 0)
    {
        perror("Something went wrong with the IP address");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    if (setsockopt(udp_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0)
    {
        perror("Joining multicast group failed");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    size_t i = 0;

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t received = recvfrom(udp_fd, &buffer, BUFFER_SIZE, 0, (sockaddr *)&sender_addr, &sender_len);

        if (received < 0)
        {
            perror("Receive failed");
        }
        else
        {
            std::cout << i << std::endl;
        }
        i++;
    }

    close(udp_fd);

    return EXIT_SUCCESS;
}
