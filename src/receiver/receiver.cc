#include "shared/message_parser.h"
#include "receiver/spsc_buffer.h"
#include "shared/limit_order_book.h"
#include "shared/benchmark.h"
#include "shared/mold_udp_64.h"
#include "shared/run_result.h"

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
    constexpr size_t kHugePageSize = 2 * 1024 * 1024;
    constexpr size_t kSpinsBeforePause = 64;

    struct alignas(fh_lob::kCacheLineSize) PacketSlot
    {
        uint64_t rx_tsc; // stamped by the RX thread as the packet leaves the socket
        uint32_t length; // bytes actually received
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
        size_t ring_full_stalls = 0; // number of times the ring went from having space to being full
        size_t max_depth = 0;
    };

    // This thread is kept deliberately trivial so socket drain rate stops depending on parse jitter.
    void ReceiveLoop(int udp_fd, PacketRing &ring, std::atomic<bool> &done, int cpu_id, size_t receive_batch, ReceiveStats &stats)
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
        bool stalled = false;
        size_t spins = 0;

        while (!done.load(std::memory_order_acquire))
        {
            PacketSlot *run = nullptr;
            const size_t index = ring.WriteIndex();
            const size_t claimed = ring.ClaimContiguous(receive_batch, &run);
            if (claimed == 0)
            {
                // Consumer is behind. Stalling rather than dropping hands the backlog down to the socket buffer, which is sized for it.
                if (!stalled)
                {
                    stats.ring_full_stalls++;
                    stalled = true;
                }
                Backoff(spins);
                continue;
            }
            stalled = false;

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

    // Everything the receiver can be asked to vary, plus the compile-time facts worth recording alongside them. This map is both what the run reads its settings out of and what gets written into the run document.
    bench::Config ReceiverConfig()
    {
        bench::Config config;
        config.AddString("multicast_ip", "239.0.0.1", "multicast group to join");
        config.AddInt("port", 12345, "UDP port to bind");
        config.AddInt("rx_cpu", 2, "CPU for the receive thread");
        config.AddInt("parse_cpu", 3, "CPU for the parse thread (a different physical core, not an SMT sibling)");
        config.AddUInt("recv_batch", 32, "packets requested per recvmmsg call");
        config.AddUInt("rcvbuf_bytes", 4 * 1024 * 1024, "SO_RCVBUF, the shock absorber behind the ring");
        config.AddUInt("warmup_messages", 10000, "messages parsed before recording starts");
        config.AddUInt("calibration_ms", 50, "sleep length per TSC calibration sample");
        config.AddUInt("lob.order_pool_capacity", 2500000, "live orders the pool can hold");
        config.AddUInt("lob.price_level_pool_capacity", 500ULL * 100 * 10000, "price levels the pool can hold");
        config.AddUInt("lob.max_locates", 10000, "stock locate codes reserved");
        config.AddUInt("lob.max_order_ref_number", 310000000, "size of the order reference table");
        config.AddUInt("lob.expected_levels_per_locate", 256, "price levels reserved per locate");
        config.AddFixedUInt("ring.capacity", kRingCapacity, "ring slots (template parameter)");
        config.AddFixedUInt("ring.slot_bytes", sizeof(PacketSlot), "bytes per ring slot");
        config.AddFixedUInt("ring.spins_before_pause", kSpinsBeforePause, "spins before the backoff PAUSE");
        return config;
    }

    int RunReceiver(const bench::Config &config, bench::RunResult &result, bool quiet)
    {
        const std::string &multicast_ip = config.GetString("multicast_ip");
        const int port = static_cast<int>(config.GetInt("port"));
        const int rx_cpu = static_cast<int>(config.GetInt("rx_cpu"));
        const int parse_cpu = static_cast<int>(config.GetInt("parse_cpu"));
        const uint64_t warmup_messages = config.GetUInt("warmup_messages");
        const size_t calibration_ms = static_cast<size_t>(config.GetUInt("calibration_ms"));

        const size_t receive_batch = static_cast<size_t>(config.GetUInt("recv_batch"));
        if (receive_batch == 0 || receive_batch > kRingCapacity)
        {
            result.Fail("--recv_batch must be between 1 and the ring capacity (" + std::to_string(kRingCapacity) + ")");
            return EXIT_FAILURE;
        }

        int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_fd < 0)
        {
            result.Fail(std::string("socket failed: ") + std::strerror(errno));
            return EXIT_FAILURE;
        }

        int reuse = 1;
        if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
        {
            result.Fail(std::string("SO_REUSEADDR failed: ") + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        // The socket buffer is the shock absorber behind the ring: it covers the window between the RX thread stalling and the consumer catching up.
        int receive_buffer = static_cast<int>(config.GetUInt("rcvbuf_bytes"));
        if (setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &receive_buffer, sizeof(receive_buffer)) < 0)
        {
            result.Fail(std::string("SO_RCVBUF failed: ") + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        // The kernel doubles and clamps what it was asked for, so the size that actually applies is read back rather than assumed.
        int applied_receive_buffer = 0;
        socklen_t applied_length = sizeof(applied_receive_buffer);
        if (getsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &applied_receive_buffer, &applied_length) == 0)
        {
            result.metrics().AddInt("rcvbuf_bytes_applied", applied_receive_buffer, "SO_RCVBUF as the kernel actually set it");
        }

        // The RX thread spins, so it must never block in the kernel.
        int flags = fcntl(udp_fd, F_GETFL, 0);
        if (flags < 0 || fcntl(udp_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            result.Fail(std::string("O_NONBLOCK failed: ") + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        struct sockaddr_in local_addr{};
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(port);
        local_addr.sin_addr = {.s_addr = INADDR_ANY};

        if (bind(udp_fd, (sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        {
            result.Fail(std::string("bind failed: ") + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        struct ip_mreq multicast_req{};
        multicast_req.imr_multiaddr = {.s_addr = 0};
        if (inet_pton(AF_INET, multicast_ip.c_str(), &(multicast_req.imr_multiaddr.s_addr)) <= 0)
        {
            result.Fail("could not parse multicast address " + multicast_ip);
            close(udp_fd);
            return EXIT_FAILURE;
        }

        if (setsockopt(udp_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0)
        {
            result.Fail(std::string("IP_ADD_MEMBERSHIP failed: ") + std::strerror(errno));
            close(udp_fd);
            return EXIT_FAILURE;
        }

        RingPtr ring = AllocateRing();
        if (!ring)
        {
            result.Fail("failed to allocate the packet ring");
            close(udp_fd);
            return EXIT_FAILURE;
        }

        size_t message_count = 0;
        size_t moldudp64_message_count = 0;
        bench::pin_to_cpu(parse_cpu);
        result.AddPinnedCpu(parse_cpu);
        result.AddPinnedCpu(rx_cpu);
        double ns_per_cycle = bench::FindNsPerCycle(calibration_ms);

        // Built before the RX thread starts: its constructor faults in gigabytes, and anything the RX thread received in the meantime would just pile up.
        fh_lob::LimitOrderBook lob(config.GetUInt("lob.order_pool_capacity"),
                                   config.GetUInt("lob.price_level_pool_capacity"),
                                   config.GetUInt("lob.max_locates"),
                                   config.GetUInt("lob.max_order_ref_number"),
                                   config.GetUInt("lob.expected_levels_per_locate"));

        // Measures queueing across the ring on top of the parse itself.
        bench::HDRHistogram end_to_end_hist;

        std::atomic<bool> done{false};
        ReceiveStats stats;
        std::thread rx_thread(ReceiveLoop, udp_fd, std::ref(*ring), std::ref(done), rx_cpu, receive_batch, std::ref(stats));

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

                if (message_count > warmup_messages) // warmup skip
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
        close(udp_fd);

        // The second calibration brackets the run: if it has drifted from the first, the cycle-to-nanosecond conversion behind every figure below did not hold.
        double second_ns_per_cycle = bench::FindNsPerCycle(calibration_ms);
        result.SetNsPerCycle(ns_per_cycle, second_ns_per_cycle);

        // start_time is only stamped once a packet arrives, so without one the interval is measured from an unset clock. Nothing was received: say so rather than record a meaningless duration and a throughput derived from it.
        const bool received_anything = stats.packets > 0;
        const int64_t elapsed_ns = received_anything ? std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count() : 0;
        const double elapsed_s = received_anything ? std::chrono::duration<double>(time_taken).count() : 0.0;
        if (!received_anything)
        {
            result.Fail("no packets received (is the sender running, and does the multicast group match?)");
        }

        bench::HDRHistogram overall_hist;
        for (size_t i = 0; i < 256; i++)
        {
            overall_hist += bench::hist[i];
        }

        result.metrics().AddInt("elapsed_ns", elapsed_ns, "first packet received to last message parsed");
        result.metrics().AddUInt("total_messages", message_count, "ITCH messages parsed, warmup included");
        result.metrics().AddUInt("recorded_messages", overall_hist.Samples(), "messages whose latency was recorded");
        result.metrics().AddUInt("moldudp64_packets", moldudp64_message_count, "MoldUDP64 packets framed by the parse thread");
        result.metrics().AddUInt("packets_received", stats.packets, "packets the RX thread took off the socket");
        result.metrics().AddUInt("ring_full_stalls", stats.ring_full_stalls, "times the RX thread found the ring full");
        result.metrics().AddUInt("max_ring_depth", stats.max_depth, "deepest the ring was observed to be");
        result.metrics().AddDouble("throughput_msgs_s", elapsed_s > 0 ? static_cast<double>(message_count) / elapsed_s : 0.0);
        result.metrics().AddDouble("throughput_packets_s", elapsed_s > 0 ? static_cast<double>(moldudp64_message_count) / elapsed_s : 0.0);

        result.AddHistogram("overall", overall_hist);
        // Same histogram shape, but the interval is socket-to-parsed rather than parse-only, so it includes time spent queued in the ring.
        result.AddHistogram("end_to_end", end_to_end_hist);
        result.AddMessageTypeHistograms(bench::hist);

        if (!quiet)
        {
            std::cerr << "* Total Time (ns): " << elapsed_ns << std::endl
                      << "* Messages received (msgs): " << message_count << std::endl
                      << "* MOLDUDP64 Packets received (packets): " << moldudp64_message_count << std::endl
                      << "* Ring full stalls (RX waiting on the parse thread): " << stats.ring_full_stalls << std::endl
                      << "* Max ring depth (slots): " << stats.max_depth << std::endl
                      << "#### Overall: " << std::endl;
            overall_hist.PrintSummary(ns_per_cycle);
            std::cerr << "#### End to end (socket to parsed): " << std::endl;
            end_to_end_hist.PrintSummary(ns_per_cycle);
            std::cerr << "* ns_per_cycle drift over the run: "
                      << (second_ns_per_cycle - ns_per_cycle) / ns_per_cycle << std::endl;
        }

        return received_anything ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

int main(int argc, char **argv)
{
    bench::Config config = ReceiverConfig();
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

    bench::RunResult result("receiver", config);
    const int status = RunReceiver(config, result, output.GetBool("quiet"));
    result.Finish();

    // The document is written whether or not the run succeeded: a failed run is a record, not a gap in the series.
    if (!bench::Emit(result, output))
        return EXIT_FAILURE;

    return status;
}
