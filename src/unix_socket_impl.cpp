#include "pulse/net/udp/udp.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "unix_socket.h"

namespace pulse::net::udp {

    constexpr size_t kPacketBufferSize = 2048;
    
    [[nodiscard("Why ask for an ErrorCode and then ignore it?")]]
    inline std::unexpected<Error> map_send_error(int err) {
        switch (err) {
            case EWOULDBLOCK:
                return make_unexpected(ErrorCode::WouldBlock);
            case EBADF:
            case ENOTSOCK:
                return make_unexpected(ErrorCode::InvalidSocket);
            case ECONNRESET:
                return make_unexpected(ErrorCode::ConnectionReset);
            default:
                return make_unexpected(ErrorCode::SendFailed);
        }
    }
    
    [[nodiscard("Why ask for an ErrorCode and then ignore it?")]]
    inline std::unexpected<Error> map_rev_error(int err) {
        switch (errno) {
            case EWOULDBLOCK:
                return make_unexpected(ErrorCode::WouldBlock);

            case EBADF:
            case ENOTSOCK:
                return make_unexpected(ErrorCode::InvalidSocket);

            default:
                return make_unexpected(ErrorCode::RecvFailed);
        }
    }

    std::expected<void, Error> SocketUnix::sendTo(const Addr& addr, const uint8_t* data, size_t length) {
        ssize_t sent = sendto(
            sockfd_,
            data,
            length,
            0,
            reinterpret_cast<const sockaddr*>(addr.sockaddrData()),
            static_cast<socklen_t>(addr.sockaddrLen())
        );

        if (sent < 0) {
            return map_send_error(errno);
        }
    
        if (sent != static_cast<ssize_t>(length)) {
            return make_unexpected(ErrorCode::PartialSend);
        }
    
        return {}; // success
    }

    std::expected<void, Error> SocketUnix::send(const uint8_t* data, size_t length) {
        ssize_t sent = ::send(sockfd_, data, length, 0);

        if (sent < 0) {
            return map_send_error(errno);
        }
    
        if (sent != static_cast<ssize_t>(length)) {
            return make_unexpected(ErrorCode::PartialSend);
        }
    
        return {}; // success
    }

    std::expected<ReceivedPacket, Error> SocketUnix::recvFrom() {
        static thread_local uint8_t buf[kPacketBufferSize];
    
        return recvFrom(ReceivedPacket{
            .data = buf,
            .size = 0,
            .capacity = kPacketBufferSize,
        });
    }

    std::expected<ReceivedPacket, Error> SocketUnix::recvFrom(ReceivedPacket&& packet) {
        if (packet.data == nullptr || packet.capacity < kPacketBufferSize) {
            return make_unexpected(ErrorCode::InvalidAddress);
        }
        
        sockaddr_storage src{};
        socklen_t srclen = sizeof(src);
    
        ssize_t received = ::recvfrom(
            sockfd_,
            packet.data,
            packet.capacity,
            0,
            reinterpret_cast<sockaddr*>(&src),
            &srclen
        );
    
        if (received < 0) {
            return map_rev_error(errno);
        }
    
        if (received == 0) {
            return make_unexpected(ErrorCode::Closed); // rare, but possible
        } else {
            packet.size = static_cast<size_t>(received);
        }
    
        auto addr_result = DecodeAddr(reinterpret_cast<sockaddr*>(&src));
        if (!addr_result) {
            return make_unexpected(addr_result.error());
        }

        const auto& addr = *addr_result;
        if (addr.port == 0) {
            return make_unexpected(ErrorCode::InvalidAddress);
        } else {
            packet.addr = std::move(*addr_result);
        }
        
        return std::move(packet);
    }

    std::expected<int, Error> SocketUnix::getHandle() const {
        if (sockfd_ == -1) {
            return make_unexpected(ErrorCode::InvalidSocket);
        }
        return sockfd_;
    }    

    void SocketUnix::close() {
        if (sockfd_ != -1) {
            ::close(sockfd_);
            sockfd_ = -1;
        }
    }

    std::expected<Addr, Error> SocketUnix::DecodeAddr(const sockaddr* addr) {
        char ip[INET6_ADDRSTRLEN];
        uint16_t port = 0;
    
        if (addr->sa_family == AF_INET) {
            auto* a = reinterpret_cast<const sockaddr_in*>(addr);
            if (!inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip))) {
                return make_unexpected(ErrorCode::InvalidAddress);
            }
            port = ntohs(a->sin_port);
        } else if (addr->sa_family == AF_INET6) {
            auto* a = reinterpret_cast<const sockaddr_in6*>(addr);
            if (!inet_ntop(AF_INET6, &a->sin6_addr, ip, sizeof(ip))) {
                return make_unexpected(ErrorCode::InvalidAddress);
            }
            port = ntohs(a->sin6_port);
        } else {
            return make_unexpected(ErrorCode::UnsupportedAddressFamily);
        }
    
        return Addr::Create(ip, port);
    }

    std::expected<std::unique_ptr<ISocket>, Error> SocketUnix::Listen(const Addr& bind_addr) {
        int family = AF_INET;
        const void* addr_ptr = nullptr;

        sockaddr_in addr4{};
        sockaddr_in6 addr6{};

        if (inet_pton(AF_INET, bind_addr.ip.c_str(), &addr4.sin_addr) == 1) {
            family = AF_INET;
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons(bind_addr.port);
            addr_ptr = &addr4;
        } else if (inet_pton(AF_INET6, bind_addr.ip.c_str(), &addr6.sin6_addr) == 1) {
            family = AF_INET6;
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(bind_addr.port);
            addr_ptr = &addr6;
        } else {
            return make_unexpected(ErrorCode::InvalidAddress);
        }

        int sockfd = ::socket(family, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            return make_unexpected(ErrorCode::SocketCreateFailed);
        }

        // Make socket non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags < 0 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
            ::close(sockfd);
            return make_unexpected(ErrorCode::SocketConfigFailed);
        }

        // Bind
        socklen_t socklen = (family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
        if (::bind(sockfd, reinterpret_cast<const sockaddr*>(addr_ptr), socklen) < 0) {
            ::close(sockfd);
            return make_unexpected(ErrorCode::BindFailed);
        }

        return std::make_unique<SocketUnix>(sockfd);
    }

    std::expected<std::unique_ptr<ISocket>, Error> SocketUnix::Dial(const Addr& remote_addr) {
        int family = AF_INET;
        sockaddr_storage remote_sock{};
        socklen_t remote_len = 0;

        if (inet_pton(AF_INET, remote_addr.ip.c_str(), &reinterpret_cast<sockaddr_in*>(&remote_sock)->sin_addr) == 1) {
            auto* addr4 = reinterpret_cast<sockaddr_in*>(&remote_sock);
            family = AF_INET;
            addr4->sin_family = AF_INET;
            addr4->sin_port = htons(remote_addr.port);
            remote_len = sizeof(sockaddr_in);
        } else if (inet_pton(AF_INET6, remote_addr.ip.c_str(), &reinterpret_cast<sockaddr_in6*>(&remote_sock)->sin6_addr) == 1) {
            auto* addr6 = reinterpret_cast<sockaddr_in6*>(&remote_sock);
            family = AF_INET6;
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = htons(remote_addr.port);
            remote_len = sizeof(sockaddr_in6);
        } else {
            return make_unexpected(ErrorCode::InvalidAddress);
        }

        int sockfd = socket(family, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            return make_unexpected(ErrorCode::SocketCreateFailed);
        }

        // Make socket non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags < 0 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
            ::close(sockfd);
            return make_unexpected(ErrorCode::SocketConfigFailed);
        }

        if (connect(sockfd, reinterpret_cast<sockaddr*>(&remote_sock), remote_len) < 0) {
            ::close(sockfd);
            return make_unexpected(ErrorCode::ConnectFailed);
        }

        try {
            return std::make_unique<SocketUnix>(sockfd);
        } catch (std::bad_alloc& err) {
            ::close(sockfd);
            return make_unexpected(ErrorCode::SocketCreateFailed, err);
        } catch (...) {
            ::close(sockfd);
            return make_unexpected(ErrorCode::Unknown, "Unknown error occurred while creating SocketUnix");
        }
    }

} // namespace pulse::net::udp
