#include "receiver/message_parser.h"
#include "shared/limit_order_book.h"
#include "receiver/benchmark.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>
#include <vector>

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <CPU pin> \n";
        return EXIT_FAILURE;
    }

    int cpu_id = atoi(argv[1]);

    int fd = open("itchmessages/12302019.NASDAQ_ITCH50", O_RDONLY, S_IRUSR | S_IWUSR);
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

    uint16_t msg_length;

    fh_lob::LimitOrderBook lob(2000000, 500 * 100 * 10000, 10000, 300000000);

    // Metrics variables
    size_t total_message_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    bench::pin_to_cpu(cpu_id);
    double ns_per_cycle = bench::FindNsPerCycle(50);

    size_t i = 0;
    while (true)
    {
        memcpy(&msg_length, file + i, sizeof(msg_length));
        uint16_t host_msg_length = ntohs(msg_length);
        i += sizeof(msg_length);

        uint8_t msg_type = static_cast<uint8_t>(*(file + i));
        uint64_t start_cycle = bench::RdtscStart();
        bool is_complete = fh_lob::ParseMessage(file + i, lob);
        uint64_t end_cycle = bench::RdtscEnd();
        i += host_msg_length;

        if (total_message_count > 10000) // warmup skip
            bench::hist[msg_type].Record(end_cycle - start_cycle);
        total_message_count++;

        if (is_complete)
            break;
    }

    // Calculate Metrics
    auto end_time = std::chrono::steady_clock::now();
    auto time_taken = end_time - start_time;

    double second_ns_per_cycle = bench::FindNsPerCycle(50);

    std::chrono::duration<double> elapsed = time_taken;
    std::cout << "* Throughput (msgs/sec): " << total_message_count / elapsed.count() << std::endl
              << "    * Total Time (ns): " << std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count() << std::endl
              << "    * Total Messages (msgs): " << total_message_count << std::endl;

    bench::HDRHistogram overall_hist;
    for (size_t i = 0; i < 256; i++)
    {
        overall_hist += bench::hist[i];
    }
    overall_hist.PrintSummary(ns_per_cycle);

    std::cout << "ns_per_cycle: " << ns_per_cycle << std::endl;
    std::cout << "Again find ns_per_cycle: " << second_ns_per_cycle << std::endl;

    std::vector<uint8_t> msg_types{'S', 'R', 'A', 'F', 'E', 'C', 'X', 'D', 'U'};
    for (uint8_t msg_type : msg_types)
    {

        std::cout << "\nmsg_type: " << msg_type << std::endl;
        bench::hist[msg_type].PrintSummary(ns_per_cycle);
    }

    overall_hist.Save("./histograms/overall_hist.txt");

    return EXIT_SUCCESS;
}