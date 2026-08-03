#include "receiver/message_parser.h"
#include "receiver/spsc_buffer.h"
#include "shared/limit_order_book.h"
#include "shared/benchmark.h"
#include "shared/mold_udp_64.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include "absl/container/flat_hash_map.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <thread>
#include <vector>

namespace
{
    constexpr size_t kMaxPacketSize = fh_lob::MoldUDP64Builder::k_max_packet_size;
    constexpr size_t kRingCapacity = 1024;
    constexpr size_t kReceiveBatch = 32;
    constexpr int kReceiveBufferBytes = 4 * 1024 * 1024;
    constexpr size_t kHugePageSize = 2 * 1024 * 1024;
    constexpr size_t kSpinsBeforePause = 64;

    struct alignas(fh_lob::kCacheLineSize) PacketSlot
    {
        uint64_t rx_tsc;    // stamped by the RX thread as the packet leaves the socket
        uint32_t length;    // bytes actually received
        char data[kMaxPacketSize];
    };

    using PacketRing = fh_lob::SPSCRingBuffer<PacketSlot, kRingCapacity>;

    struct RingDeleter
    {
        void operator()(PacketRing *ring) const
        {
            ring->~PacketRing();
            std::free(ring);
        }
    };
    using RingPtr = std::unique_ptr<PacketRing, RingDeleter>;

    RingPtr AllocateRing()
    {
        const size_t bytes = (sizeof(PacketRing) + kHugePageSize - 1) & ~(kHugePageSize - 1);
        void *memory = std::aligned_alloc(kHugePageSize, bytes);
        if (memory == nullptr)
        {
            return RingPtr{};
        }

        if (madvise(memory, bytes, MADV_HUGEPAGE) != 0)
        {
            perror("madvise(MADV_HUGEPAGE) failed, continuing with 4 KiB pages");
        }
        memset(memory, 0, bytes);

        return RingPtr{new (memory) PacketRing()};
    }

    inline void Backoff(size_t &spins)
    {
        if (++spins >= kSpinsBeforePause)
        {
            __builtin_ia32_pause();
        }
    }

    // Written by the RX thread and read by main with no atomics and no mutex, and is only safe because join() supplies the happens-before edge.
    struct ReceiveStats
    {
        std::chrono::steady_clock::time_point start_time;
        size_t packets = 0;
        size_t ring_full_stalls = 0;
        size_t max_depth = 0;
    };

    // This thread is kept deliberately trivial so socket drain rate stops depending on parse jitter.
    void ReceiveLoop(int udp_fd, PacketRing &ring, std::atomic<bool> &done, int cpu_id, ReceiveStats &stats)
    {
        bench::pin_to_cpu(cpu_id);

        // A slot's address never changes, so the message headers are built once here and indexed exactly like the ring. The steady-state call then has no per-packet setup at all -- it only reads msg_len back out. msg_name stays null: the sender address is never used.
        PacketSlot *slots = ring.Slots();
        std::vector<iovec> iovecs(kRingCapacity);
        std::vector<mmsghdr> headers(kRingCapacity);
        for (size_t i = 0; i < kRingCapacity; i++)
        {
            iovecs[i].iov_base = slots[i].data;
            iovecs[i].iov_len = sizeof(slots[i].data);
            headers[i].msg_hdr.msg_iov = &iovecs[i];
            headers[i].msg_hdr.msg_iovlen = 1;
        }

        bool started = false;
        size_t spins = 0;

        while (!done.load(std::memory_order_acquire))
        {
            PacketSlot *run = nullptr;
            const size_t index = ring.WriteIndex();
            const size_t claimed = ring.ClaimContiguous(kReceiveBatch, &run);
            if (claimed == 0)
            {
                // Consumer is behind. Stalling rather than dropping hands the backlog down to the socket buffer, which is sized for it.
                stats.ring_full_stalls++;
                Backoff(spins);
                continue;
            }

            const int count = recvmmsg(udp_fd, &headers[index], claimed, MSG_DONTWAIT, nullptr);
            if (count <= 0)
            {
                if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                {
                    perror("Receive failed");
                    break;
                }
                Backoff(spins);
                continue;
            }
            spins = 0;

            const uint64_t rx_tsc = bench::RdtscEnd();
            for (int i = 0; i < count; i++)
            {
                run[i].rx_tsc = rx_tsc;
                run[i].length = headers[index + static_cast<size_t>(i)].msg_len;
            }
            ring.CommitBatch(static_cast<size_t>(count));

            if (!started)
            {
                stats.start_time = std::chrono::steady_clock::now();
                started = true;
            }
            stats.packets += static_cast<size_t>(count);

            // Reads the consumer's counter, so it is sampled once per syscall rather than once per packet.
            const size_t depth = ring.Depth();
            if (depth > stats.max_depth)
            {
                stats.max_depth = depth;
            }
        }

        // Unblocks the consumer if we left because of a socket error rather than because it told us the session was over.
        done.store(true, std::memory_order_release);
    }
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <multicast_ip> <port> <rx_cpu> <parse_cpu> \n"
                  << "The two CPUs must be different physical cores, not hyperthread siblings of each other (e.g. \"239.0.0.1\" 12345 2 3)\n";
        return EXIT_FAILURE;
    }

    const char *multicast_ip = argv[1];
    int port = atoi(argv[2]);
    int rx_cpu = atoi(argv[3]);
    int parse_cpu = atoi(argv[4]);

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

    // The socket buffer is the shock absorber behind the ring: it covers the window between the RX thread stalling and the consumer catching up.
    int receive_buffer = kReceiveBufferBytes;
    if (setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &receive_buffer, sizeof(receive_buffer)) < 0)
    {
        perror("Setting SO_RCVBUF failed");
        close(udp_fd);
        return EXIT_FAILURE;
    }

    // The RX thread spins, so it must never block in the kernel.
    int flags = fcntl(udp_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(udp_fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("Setting O_NONBLOCK failed");
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

    RingPtr ring = AllocateRing();
    if (!ring)
    {
        std::cerr << "Failed to allocate the packet ring" << std::endl;
        close(udp_fd);
        return EXIT_FAILURE;
    }

    size_t message_count = 0;
    size_t moldudp64_message_count = 0;
    bench::pin_to_cpu(parse_cpu);
    double ns_per_cycle = bench::FindNsPerCycle(50);

    // Built before the RX thread starts: its constructor faults in gigabytes, and anything the RX thread received in the meantime would just pile up.
    fh_lob::LimitOrderBook lob(2500000, 500 * 100 * 10000, 10000, 310000000);

    // Measures queueing across the ring on top of the parse itself.
    bench::HDRHistogram end_to_end_hist;

    std::atomic<bool> done{false};
    ReceiveStats stats;
    std::thread rx_thread(ReceiveLoop, udp_fd, std::ref(*ring), std::ref(done), rx_cpu, std::ref(stats));

    size_t spins = 0;
    while (true)
    {
        const PacketSlot *slot = ring->PeekRead();
        if (slot == nullptr)
        {
            if (done.load(std::memory_order_acquire))
            {
                break; // ring drained and the RX thread has given up
            }
            Backoff(spins);
            continue;
        }
        spins = 0;

        if (slot->length < sizeof(fh_lob::MoldUDP64Header))
        {
            ring->CommitRead(); // runt, not a packet we can frame
            continue;
        }

        fh_lob::MoldUDP64View moldudp64_packet(slot->data, slot->length);
        if (moldudp64_packet.is_end_of_session())
        {
            break;
        }

        for (const fh_lob::MoldUDP64View::Message &msg : moldudp64_packet)
        {
            uint8_t msg_type = static_cast<uint8_t>(*(msg.payload));
            uint64_t start_cycle = bench::RdtscStart();
            fh_lob::ParseMessage(msg.payload, lob);
            uint64_t end_cycle = bench::RdtscEnd();

            if (message_count > 10000) // warmup skip
            {
                bench::hist[msg_type].Record(end_cycle - start_cycle);
                end_to_end_hist.Record(end_cycle - slot->rx_tsc);
            }

            message_count++;
        }

        moldudp64_message_count++;
        ring->CommitRead();
    }

    // Calculate Metrics
    auto end_time = std::chrono::steady_clock::now();
    done.store(true, std::memory_order_release);
    rx_thread.join();
    auto time_taken = end_time - stats.start_time;

    double second_ns_per_cycle = bench::FindNsPerCycle(50);

    std::cout << "=====TIME TAKEN=====\n"
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count()
              << "\n=====TOTAL MESSAGE COUNT=====\n"
              << message_count
              << "\n=====TOTAL MOLDUDP64 MESSAGES\n"
              << moldudp64_message_count
              << "\n=====RING FULL STALLS=====\n"
              << stats.ring_full_stalls
              << "\n=====MAX RING DEPTH=====\n"
              << stats.max_depth
              << std::endl;

    bench::HDRHistogram overall_hist;
    for (size_t i = 0; i < 256; i++)
    {
        overall_hist += bench::hist[i];
    }
    std::cout << "#### Overall: \n";
    overall_hist.PrintSummary(ns_per_cycle);

    // Same histogram format, but the interval is socket-to-parsed rather than parse-only, so it includes time spent queued in the ring.
    std::cout << "\n#### End to end (socket to parsed): \n";
    end_to_end_hist.PrintSummary(ns_per_cycle);

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
