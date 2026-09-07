#include "shared/mold_udp_64.h"
#include "shared/run_result.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cerrno>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cstring>
#include <chrono> // for sleep
#include <thread> // for sleep
#include <cassert>

namespace
{
    bench::Config SenderConfig()
    {
        bench::Config config;
        config.AddString("multicast_ip", "239.0.0.1", "multicast group to send to");
        config.AddInt("port", 12345, "UDP port to send to");
        config.AddString("input", "", "ITCH message file to replay");
        config.AddString("session", "Session123", "MoldUDP64 session name");
        config.AddInt("ttl", 1, "IP_MULTICAST_TTL");
        config.AddUInt("end_of_session_repeats", 100, "end-of-session packets sent to close the run");
        config.AddUInt("end_of_session_gap_ms", 1, "pause between end-of-session packets");
        return config;
    }

    int RunSender(const bench::Config &config, bench::RunResult &result, bool quiet)
    {
        const std::string &input_path = config.GetString("input");
        if (input_path.empty())
        {
            result.Fail("no --input file given");
            return EXIT_FAILURE;
        }

        // socket setup
        const std::string &multicast_ip = config.GetString("multicast_ip");
        const int port = static_cast<int>(config.GetInt("port"));

        int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_fd < 0)
        {
            result.Fail(std::string("socket failed: ") + std::strerror(errno));
            return EXIT_FAILURE;
        }

        unsigned char ttl = static_cast<unsigned char>(config.GetInt("ttl"));
        if (setsockopt(udp_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
        {
            result.Fail(std::string("IP_MULTICAST_TTL failed: ") + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        struct sockaddr_in multicast_addr{};
        multicast_addr.sin_family = AF_INET;
        multicast_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, multicast_ip.c_str(), &(multicast_addr.sin_addr.s_addr)) <= 0)
        {
            result.Fail("could not parse multicast address " + multicast_ip);
            close(udp_fd);
            return EXIT_FAILURE;
        }

        // mmap setup
        int fd = open(input_path.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
        if (fd < 0)
        {
            result.Fail("could not open " + input_path + ": " + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1)
        {
            result.Fail(std::string("fstat failed: ") + std::strerror(errno));
            close(fd);
            close(udp_fd);
            return EXIT_FAILURE;
        }

        size_t file_size = static_cast<size_t>(sb.st_size);
        result.SetInputFile(input_path);

        const char *file = static_cast<const char *>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (file == MAP_FAILED)
        {
            result.Fail(std::string("mmap failed: ") + std::strerror(errno));
            close(fd);
            close(udp_fd);
            return EXIT_FAILURE;
        }

        if (madvise(const_cast<char *>(file), sb.st_size, MADV_SEQUENTIAL) == -1)
        {
            perror("madvise");
        }

        // send
        fh_lob::MoldUDP64Builder builder(config.GetString("session"), 1);
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
                    result.Fail(std::string("sendto failed: ") + std::strerror(errno));
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
                result.Fail(std::string("sendto failed: ") + std::strerror(errno));
                close(udp_fd);
                return EXIT_FAILURE;
            };

            moldudp64_msg_count++;
        }

        const size_t terminating_messages = config.GetUInt("end_of_session_repeats");
        const auto terminating_gap = std::chrono::milliseconds(config.GetUInt("end_of_session_gap_ms"));
        const char *end_of_session = builder.FinalizeEndOfSession();
        const size_t end_of_session_size = builder.size();

        for (size_t i = 0; i < terminating_messages; i++)
        {
            if (sendto(udp_fd, end_of_session, end_of_session_size, 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
            {
                result.Fail(std::string("sending end of session failed: ") + std::strerror(errno));
                close(udp_fd);
                return EXIT_FAILURE;
            };

            std::this_thread::sleep_for(terminating_gap);
        }

        munmap(const_cast<char *>(file), file_size);
        close(fd);
        close(udp_fd);

        // Calculate Metrics
        auto end_time = std::chrono::steady_clock::now();
        auto time_taken = end_time - start_time;
        const int64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count();

        // The receiver's drop rate is (sent - received) / sent, so these two counts are half of an online result and belong in a document rather than in a shell variable carried forward by hand.
        result.metrics().AddInt("elapsed_ns", elapsed_ns, "replay wall time, end-of-session pause included");
        result.metrics().AddUInt("messages_sent", msg_count, "ITCH messages sent");
        result.metrics().AddUInt("packets_sent", moldudp64_msg_count, "MoldUDP64 packets sent");
        result.metrics().AddUInt("end_of_session_packets", terminating_messages, "end-of-session packets sent");

        if (!quiet)
        {
            std::cerr << "* Messages sent (msgs): " << msg_count << std::endl
                      << "* MOLDUDP64 Packets sent (packets): " << moldudp64_msg_count << std::endl
                      << "* Total Time (ns): " << elapsed_ns << std::endl;
        }

        return EXIT_SUCCESS;
    }
}

int main(int argc, char *argv[])
{
    bench::Config config = SenderConfig();
    bench::ValueMap output = bench::DefaultOutputOptions();

    // Rendered before parsing, so the defaults it quotes are the defaults and not whatever a half-applied command line left behind.
    const std::string usage = bench::UsageString(argv[0], {&config, &output});
    if (bench::HelpRequested(argc, argv))
    {
        std::cout << usage;
        return EXIT_SUCCESS;
    }

    std::string error;
    if (!bench::ParseArgs(argc, argv, {&config, &output}, &error))
    {
        std::cerr << argv[0] << ": " << error << "\n\n"
                  << usage;
        return EXIT_FAILURE;
    }

    bench::RunResult result("sender", config);
    const int status = RunSender(config, result, output.GetBool("quiet"));
    result.Finish();

    if (!bench::Emit(result, output))
        return EXIT_FAILURE;

    return status;
}
