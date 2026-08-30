#include "shared/mold_udp_64.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cstring>
#include <chrono> // for sleep
#include <thread> // for sleep
#include <cassert>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <multicast_ip> <port> <itch_messages_file> \n"; // e.g., "239.0.0.1" 12345 "itchmessages/12302019.NASDAQ_ITCH50"
        return EXIT_FAILURE;
    }

    // socket setup
    const char *multicast_ip = argv[1];
    int port = atoi(argv[2]);

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

    struct sockaddr_in multicast_addr{};
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, multicast_ip, &(multicast_addr.sin_addr.s_addr)) <= 0)
    {
        perror("Something went wrong with the IP address");
        return EXIT_FAILURE;
    }

    // mmap setup
    const char *itch_messages_file = argv[3];
    int fd = open(itch_messages_file, O_RDONLY, S_IRUSR | S_IWUSR);
    struct stat sb;

    if (fstat(fd, &sb) == -1)
    {
        perror("Couldn't get file size.\n");
        return EXIT_FAILURE;
    }

    size_t file_size = static_cast<size_t>(sb.st_size);
    std::cout << "file size is " << file_size << std::endl;

    const char *file = static_cast<const char *>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file == MAP_FAILED)
    {
        perror("MAP_FAILED\n");
        return EXIT_FAILURE;
    }

    if (madvise(const_cast<char *>(file), sb.st_size, MADV_SEQUENTIAL) == -1)
    {
        perror("madvise");
    }

    // send
    const std::string session = "Session123";
    fh_lob::MoldUDP64Builder builder(session, 1);
    const char *cursor = file;
    const char *file_end = file + file_size;

    size_t msg_count = 0;
    size_t moldudp64_msg_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (cursor + sizeof(uint16_t) <= file_end)
    {
        uint16_t net_length;
        memcpy(&net_length, cursor, sizeof(net_length));
        const uint16_t length = ntohs(net_length);
        assert(length < fh_lob::MoldUDP64Builder::k_max_packet_size);
        const char *payload = cursor + sizeof(uint16_t);

        if (!builder.TryAppend(payload, length))
        {
            if (sendto(udp_fd, builder.Finalize(), builder.size(), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
            {
                perror("Failed to send message");
                close(udp_fd);
                return EXIT_FAILURE;
            };
            builder.Reset();
            builder.TryAppend(payload, length);

            moldudp64_msg_count++;
        }

        msg_count++;
        cursor = payload + length;
    }

    if (!builder.Empty())
    {
        if (sendto(udp_fd, builder.Finalize(), builder.size(), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
        {
            perror("Failed to send message");
            close(udp_fd);
            return EXIT_FAILURE;
        };

        moldudp64_msg_count++;
    }

    constexpr size_t terminating_messages = 100;
    const char *end_of_session = builder.FinalizeEndOfSession();
    const size_t end_of_session_size = builder.size();

    for (size_t i = 0; i < terminating_messages; i++)
    {
        if (sendto(udp_fd, end_of_session, end_of_session_size, 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
        {
            perror("Failed to send end of session message");
            close(udp_fd);
            return EXIT_FAILURE;
        };

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    munmap(const_cast<char *>(file), file_size);
    close(fd);

    // Calculate Metrics
    auto end_time = std::chrono::steady_clock::now();
    auto time_taken = end_time - start_time;
    std::cout << "=====TIME TAKEN=====\n"
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count()
              << "\n=====MESSAGE COUNT=====\n"
              << msg_count
              << "\n=====MoldUDP64 MESSAGE COUNT=====\n"
              << moldudp64_msg_count
              << std::endl;

    return EXIT_SUCCESS;
}
