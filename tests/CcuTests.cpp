#include <pulse/net/udp/udp.h>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>

using namespace pulse::net::udp;

int handleServer() {
    // We're an integration test so we'll use the get_socket_factory() to get the socket factory
    // and create a socket. This is the same as the client code.
    auto factory = get_socket_factory();

    auto serverAddrResult = Addr::Create("127.0.0.1", 9000);
    if (!serverAddrResult) {
        std::cerr << "Failed to create server address: " << to_string(serverAddrResult) << std::endl;
        return 1;
    }
    auto& serverAddr = *serverAddrResult;
    auto sockResult = factory->listen(serverAddr);
    if (!sockResult) {
        std::cerr << "Failed to bind server: " << to_string(sockResult) << std::endl;
        return 1;
    }

    auto& server = *sockResult;

    std::unordered_map<std::string, int> clientDatagramCount;

    while (true) {
        auto packet = server->recvFrom();
        if (!packet.has_value()) {
            if (packet.error() == ErrorCode::WouldBlock) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            std::cerr << "recvFrom failed: " << to_string(packet) << std::endl;
            continue;
        }

        const auto& [data, length, addr] = *packet;
        std::string clientKey = addr.ip + ":" + std::to_string(addr.port);
        clientDatagramCount[clientKey]++;

        auto result = server->sendTo(addr, data, length);
        if (!result) {
            std::cerr << "sendTo failed: " << to_string(result) << std::endl;
        }
    }

    return 0;
}

void sendDatagrams(const std::string& serverIp, uint16_t serverPort,
                   std::atomic<uint64_t>* datagramCount,
                   std::atomic<uint64_t>* datagramTimeoutCount,
                   std::atomic<bool>* stopFlag) {
    // We're an integration test so we'll use the get_socket_factory() to get the socket factory
    // and create a socket. This is the same as the client code.
    auto factory = get_socket_factory();

    auto serverAddrResult = Addr::Create(serverIp, serverPort);
    if (!serverAddrResult) {
        std::cerr << "Failed to create server address: " << to_string(serverAddrResult) << std::endl;
        return;
    }
    auto& serverAddr = *serverAddrResult;
    auto sockResult = factory->dial(serverAddr);
    if (!sockResult) {
        std::cerr << "Failed to dial server: " << to_string(sockResult) << std::endl;
        return;
    }

    auto& client = *sockResult;

    bool waitingForResponse = false;
    auto timeSinceLastSend = std::chrono::steady_clock::now();
    auto maxWaitTime = std::chrono::milliseconds(1000 / 48); // 24Hz tick

    std::vector<uint8_t> message = {'h', 'e', 'l', 'l', 'o'};

    while (!stopFlag->load()) {
        if (waitingForResponse &&
            std::chrono::steady_clock::now() < timeSinceLastSend + maxWaitTime) {
            auto maybe = client->recvFrom();
            if (maybe.has_value()) {
                waitingForResponse = false;
                datagramCount->fetch_add(1);
            } else if (maybe.error() != ErrorCode::WouldBlock) {
                std::cerr << "recvFrom failed: " << to_string(maybe) << std::endl;
                waitingForResponse = false;
            }
        } else if (waitingForResponse) {
            datagramTimeoutCount->fetch_add(1);
            waitingForResponse = false;
        } else {
            std::this_thread::sleep_for(maxWaitTime - (std::chrono::steady_clock::now() - timeSinceLastSend));
            timeSinceLastSend = std::chrono::steady_clock::now();

            auto result = client->send(message.data(), message.size());
            if (!result) {
                std::cerr << "send failed: " << to_string(result) << std::endl;
            } else {
                waitingForResponse = true;
            }
        }
    }
}

int handleClient(int concurrentClients = 10) {
    const int numClients = concurrentClients;
    std::atomic<uint64_t> datagramsSent(0);
    std::atomic<uint64_t> datagramsTimedOut(0);
    std::atomic<bool> stopFlag(false);

    std::vector<std::thread> clientThreads;

    auto startTime = std::chrono::steady_clock::now();
    auto stopSignal = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    std::cout << "Sending datagrams for 10 seconds on " << numClients << " clients..." << std::endl;

    for (int i = 0; i < numClients; ++i) {
        clientThreads.emplace_back(sendDatagrams, "127.0.0.1", 9000, &datagramsSent, &datagramsTimedOut, &stopFlag);
    }

    std::this_thread::sleep_until(stopSignal);
    stopFlag.store(true);

    for (auto& thread : clientThreads) {
        if (thread.joinable()) thread.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    std::cout << "All clients stopped." << std::endl;
    std::cout << "Elapsed time: " << elapsedTime << " seconds\n"
              << "Total datagrams sent: " << datagramsSent.load() << "\n"
              << "Average datagrams/sec: " << (datagramsSent.load() / elapsedTime) << "\n"
              << "Average per client/sec: " << ((datagramsSent.load() / numClients) / elapsedTime) << "\n"
              << "Total timeouts: " << datagramsTimedOut.load() << "\n"
              << "Timeouts/sec: " << (datagramsTimedOut.load() / elapsedTime) << "\n"
              << "Timeouts per client: " << (datagramsTimedOut.load() / numClients) << std::endl;

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--server") {
        return handleServer();
    } else if (argc > 1 && std::string(argv[1]) == "--client") {
        int numClients = (argc > 2) ? std::stoi(argv[2]) : 10;
        return handleClient(numClients);
    } else {
        std::cout << "Usage: " << argv[0] << " [--server | --client [numClients]]\n";
        return 1;
    }
}
