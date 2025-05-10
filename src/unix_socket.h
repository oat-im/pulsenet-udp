#pragma once

#include <pulse/net/udp/socket_factory.h>
#include <pulse/net/udp/udp.h>
#include <pulse/net/udp/error_code.h>
#include <pulse/net/udp/udp_addr.h>

struct sockaddr;

namespace pulse::net::udp {

    class SocketUnix : public ISocket {
    public:
        SocketUnix(int sockfd) : sockfd_(sockfd) {}
        ~SocketUnix() override {
            close();
        }
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<void, Error> sendTo(const Addr& addr, const uint8_t* data, size_t length) override;
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<void, Error> send(const uint8_t* data, size_t length) override;
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<ReceivedPacket, Error> recvFrom() override;
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<ReceivedPacket, Error> recvFrom(ReceivedPacket&& packet) override;
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<int, Error> getHandle() const override;
    
        void close() override;

        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<std::unique_ptr<ISocket>, Error> Listen(const Addr& bindAddr);
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<std::unique_ptr<ISocket>, Error> Dial(const Addr& remoteAddr);
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<Addr, Error> DecodeAddr(const sockaddr* addr);
    
    private:
        int sockfd_;
        
    };

} // namespace pulse::net::udp