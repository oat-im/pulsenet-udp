#include <pulse/net/udp/udp.h>
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

#include "win_socket.h"

#pragma comment(lib, "ws2_32.lib")

namespace pulse::net::udp {

    constexpr size_t PACKET_BUFFER_SIZE = 2048;

    static std::atomic<int> wsaRefCount{0};
    static std::mutex wsaMutex;

    [[nodiscard("You're ignoring the possibility of failure.")]]
    static std::expected<void, Error> init_wsa() {
        std::lock_guard<std::mutex> lock(wsaMutex);
        if (wsaRefCount == 0) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                return make_unexpected(ErrorCode::WSAStartupFailed);
            }
        }
        ++wsaRefCount;
        return {};
    }


    static void cleanup_wsa() {
        std::lock_guard<std::mutex> lock(wsaMutex);
        if (wsaRefCount > 0) {
            wsaRefCount--;
            if (wsaRefCount == 0) {
                WSACleanup();
            }
        }
    }

    [[nodiscard("Why ask for an ErrorCode and then ignore it?")]]
    inline std::unexpected<Error> map_wsa_send_error(int err) {
        switch (err) {
            case WSAEWOULDBLOCK: return make_unexpected(ErrorCode::WouldBlock);
            case WSAENOTSOCK:
            case WSAEBADF:       return make_unexpected(ErrorCode::InvalidSocket);
            case WSAECONNRESET:  return make_unexpected(ErrorCode::ConnectionReset);
            default:             return make_unexpected(ErrorCode::SendFailed);
        }
    }
    
    [[nodiscard("Why ask for an ErrorCode and then ignore it?")]]
    inline std::unexpected<Error> map_wsa_receive_error(int err) {
        switch (err) {
            case WSAEWOULDBLOCK: return make_unexpected(ErrorCode::WouldBlock);
            case WSAENOTSOCK:
            case WSAEBADF:       return make_unexpected(ErrorCode::InvalidSocket);
            default:             return make_unexpected(ErrorCode::RecvFailed);
        }
    }

    SocketWindows::SocketWindows(SOCKET sock) : sock_(sock) {
        // Increase socket buffer sizes but don't start receiving thread
        int sendBufSize = 4 * 1024 * 1024; // 4MB
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));
        
        // We still need a reasonable receive buffer for any responses
        int recvBufSize = 1 * 1024 * 1024; // 1MB is enough for client
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, sizeof(recvBufSize));
        
        // Disable connection reset behavior
        BOOL bNewBehavior = FALSE;
        DWORD dwBytesReturned = 0;
        WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), 
                NULL, 0, &dwBytesReturned, NULL, NULL);
    }

    SocketWindows::~SocketWindows() {
        close();
        cleanup_wsa();
    }

    std::expected<void, Error> SocketWindows::sendTo(const Addr& addr, const uint8_t* data, size_t length) {
        int sent = ::sendto(
            sock_,
            reinterpret_cast<const char*>(data),
            static_cast<int>(length),
            0,
            reinterpret_cast<const sockaddr*>(addr.sockaddrData()),
            static_cast<int>(addr.sockaddrLen())
        );
    
        if (sent == SOCKET_ERROR) {
            int err = WSAGetLastError();
            return map_wsa_send_error(err);
        }
    
        if (sent != static_cast<int>(length)) {
            return make_unexpected(ErrorCode::PartialSend);
        }
    
        return {};
    }

    std::expected<void, Error> SocketWindows::send(const uint8_t* data, size_t length) {
        int sent = ::send(sock_, reinterpret_cast<const char*>(data), static_cast<int>(length), 0);
    
        if (sent == SOCKET_ERROR) {
            int err = WSAGetLastError();
            return map_wsa_send_error(err);
        }
    
        if (sent != static_cast<int>(length)) {
            return make_unexpected(ErrorCode::PartialSend);
        }
    
        return {};
    }

    std::expected<ReceivedPacket, Error> SocketWindows::recvFrom() {
        static thread_local uint8_t buf[PACKET_BUFFER_SIZE];
        sockaddr_storage src{};
        int srclen = sizeof(src);
    
        int received = ::recvfrom(
            sock_,
            reinterpret_cast<char*>(buf),
            sizeof(buf),
            0,
            reinterpret_cast<sockaddr*>(&src),
            &srclen
        );
    
        if (received == SOCKET_ERROR) {
            int err = WSAGetLastError();
            return map_wsa_receive_error(err);
        }
    
        auto addr = DecodeAddr(reinterpret_cast<sockaddr*>(&src));
        if (!addr) {
            return std::unexpected(addr.error());
        }
    
        if (addr->port == 0) {
            return make_unexpected(ErrorCode::InvalidAddress);
        }
    
        return ReceivedPacket{
            .data = reinterpret_cast<const uint8_t*>(buf),
            .length = static_cast<size_t>(received),
            .addr = std::move(*addr)
        };
    }
    
    std::expected<int, Error> SocketWindows::getHandle() const {
        if (sock_ == INVALID_SOCKET) {
            return make_unexpected(ErrorCode::InvalidSocket);
        }
        return static_cast<int>(sock_);
    }
    

    void SocketWindows::close() {
        if (sock_ != INVALID_SOCKET) {
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
        }
    }

    std::expected<Addr, Error> SocketWindows::DecodeAddr(const sockaddr* addr) {
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

    std::expected<std::unique_ptr<ISocket>, Error> SocketWindows::Listen(const Addr& bindAddr) {
        if (auto err = init_wsa(); !err) {
            return std::unexpected(err.error());
        }

        int family = AF_INET;
        const void* addrPtr = nullptr;

        sockaddr_in addr4{};
        sockaddr_in6 addr6{};

        if (inet_pton(AF_INET, bindAddr.ip.c_str(), &addr4.sin_addr) == 1) {
            family = AF_INET;
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons(bindAddr.port);
            addrPtr = &addr4;
        } else if (inet_pton(AF_INET6, bindAddr.ip.c_str(), &addr6.sin6_addr) == 1) {
            family = AF_INET6;
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(bindAddr.port);
            addrPtr = &addr6;
        } else {
            return make_unexpected(ErrorCode::InvalidAddress);
        }

        SOCKET sock = socket(family, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            return make_unexpected(ErrorCode::SocketCreateFailed);
        }

        u_long mode = 1;
        if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
            closesocket(sock);
            return make_unexpected(ErrorCode::SocketConfigFailed);
        }

        int result = bind(
            sock,
            reinterpret_cast<const sockaddr*>(addrPtr),
            (family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)
        );

        if (result < 0) {
            closesocket(sock);
            return make_unexpected(ErrorCode::BindFailed);
        }

        return std::make_unique<SocketWindows>(sock);
    }

    std::expected<std::unique_ptr<ISocket>, Error> SocketWindows::Dial(const Addr& remoteAddr) {
        if (auto err = init_wsa(); !err) {
            return std::unexpected(err.error());
        }

        int family = AF_INET;
        sockaddr_storage remoteSock{};
        int remoteLen = 0;

        if (InetPtonA(AF_INET, remoteAddr.ip.c_str(), &reinterpret_cast<sockaddr_in*>(&remoteSock)->sin_addr) == 1) {
            auto* addr4 = reinterpret_cast<sockaddr_in*>(&remoteSock);
            family = AF_INET;
            addr4->sin_family = AF_INET;
            addr4->sin_port = htons(remoteAddr.port);
            remoteLen = sizeof(sockaddr_in);
        } else if (InetPtonA(AF_INET6, remoteAddr.ip.c_str(), &reinterpret_cast<sockaddr_in6*>(&remoteSock)->sin6_addr) == 1) {
            auto* addr6 = reinterpret_cast<sockaddr_in6*>(&remoteSock);
            family = AF_INET6;
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = htons(remoteAddr.port);
            remoteLen = sizeof(sockaddr_in6);
        } else {
            return make_unexpected(ErrorCode::InvalidAddress);
        }

        SOCKET sock = socket(family, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET) {
            return make_unexpected(ErrorCode::SocketCreateFailed);
        }

        // Non-blocking
        u_long mode = 1;
        if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
            closesocket(sock);
            return make_unexpected(ErrorCode::SocketConfigFailed);
        }

        if (connect(sock, reinterpret_cast<sockaddr*>(&remoteSock), remoteLen) == SOCKET_ERROR) {
            closesocket(sock);
            return make_unexpected(ErrorCode::ConnectFailed);
        }

        return std::make_unique<SocketWindows>(sock);
    }


} // namespace pulse::net::udp