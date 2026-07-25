#include "receiver/message_parser.h"
#include "shared/limit_order_book.h"
#include "shared/benchmark.h"
#include "shared/mold_udp_64.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "absl/container/flat_hash_map.h"

#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>

#define BUFFER_SIZE 1472

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <multicast_ip> <port> <CPU pin> \n"; // e.g., "239.0.0.1" 12345 3
        return EXIT_FAILURE;
    }

    const char *multicast_ip = argv[1];
    int port = atoi(argv[2]);
    int cpu_id = atoi(argv[3]);

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

    struct sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr = {.s_addr = INADDR_ANY};

    if (bind(udp_fd, (sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        perror("Binding to socket failed");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    struct ip_mreq multicast_req{};
    multicast_req.imr_multiaddr = {.s_addr = 0};
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


    // Metrics variables
    size_t message_count = 0;
    size_t moldudp64_message_count = 0;
    bench::pin_to_cpu(cpu_id);
    double ns_per_cycle = bench::FindNsPerCycle(50);
    
    
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    fh_lob::LimitOrderBook lob(2500000, 500 * 100 * 10000, 10000, 310000000);

    ssize_t received = recvfrom(udp_fd, &buffer, BUFFER_SIZE, 0, (sockaddr *)&sender_addr, &sender_len);
    if (received < 0)
    {
        perror("Receive failed");
    }
    auto start_time = std::chrono::steady_clock::now();

    while (true)
    {
        fh_lob::MoldUDP64View moldudp64_packet(buffer, received);
        if(moldudp64_packet.is_end_of_session())
        {
            break;
        }

        for(const fh_lob::MoldUDP64View::Message& msg : moldudp64_packet)
        {
            uint8_t msg_type = static_cast<uint8_t>(*(msg.payload));
            uint64_t start_cycle = bench::RdtscStart();
            fh_lob::ParseMessage(msg.payload, lob);
            uint64_t end_cycle = bench::RdtscEnd();

            if (message_count > 10000) // warmup skip
                bench::hist[msg_type].Record(end_cycle - start_cycle);

            message_count++;
        }
        
        moldudp64_message_count++;
        
        ssize_t received = recvfrom(udp_fd, &buffer, BUFFER_SIZE, 0, (sockaddr *)&sender_addr, &sender_len);
        if (received < 0)
        {
            perror("Receive failed");
            break;
        }
    }

    // Calculate Metrics
    auto end_time = std::chrono::steady_clock::now();
    auto time_taken = end_time - start_time;

    double second_ns_per_cycle = bench::FindNsPerCycle(50);

    std::cout << "=====TIME TAKEN=====\n"
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count()
              << "\n=====TOTAL MESSAGE COUNT=====\n"
              << message_count
              << "\n=====TOTAL MOLDUDP64 MESSAGES\n"
              << moldudp64_message_count
              << std::endl;

    bench::HDRHistogram overall_hist;
    for (size_t i = 0; i < 256; i++)
    {
        overall_hist += bench::hist[i];
    }
    std::cout << "#### Overall: \n";
    overall_hist.PrintSummary(ns_per_cycle);

    std::cout << "ns_per_cycle: " << ns_per_cycle << std::endl;
    std::cout << "Again find ns_per_cycle: " << second_ns_per_cycle << std::endl;

    std::vector<uint8_t> msg_types{'S', 'R', 'A', 'F', 'E', 'C', 'X', 'D', 'U'};
    for (uint8_t msg_type : msg_types)
    {

        std::cout << "\n#### msg_type " << msg_type << ": " << std::endl;
        bench::hist[msg_type].PrintSummary(ns_per_cycle);
    }

    overall_hist.Save("./histograms/overall_hist.txt");

    close(udp_fd);

    return EXIT_SUCCESS;
}
