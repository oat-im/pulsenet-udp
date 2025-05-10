#include "pulse/net/udp/udp_addr.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

namespace pulse::net::udp {

    const char* Addr::kAnyIPv4 = "0.0.0.0";
    const char* Addr::kAnyIPv6 = "::";

    Addr::Addr(const std::string& ip_str, uint16_t port)
        : ip(ip_str), port(port)
    {
        sockaddr_in* addr4 = reinterpret_cast<sockaddr_in*>(storage_);
        sockaddr_in6* addr6 = reinterpret_cast<sockaddr_in6*>(storage_);

        if (InetPtonA(AF_INET, ip_str.c_str(), &addr4->sin_addr) == 1) {
            addr4->sin_family = AF_INET;
            addr4->sin_port = htons(port);
        } else if (InetPtonA(AF_INET6, ip_str.c_str(), &addr6->sin6_addr) == 1) {
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = htons(port);
        } else {
            throw std::invalid_argument("Invalid IP address: " + ip_str);
        }
    }

    const void* Addr::sockaddrData() const {
        return static_cast<const void*>(storage_);
    }

    size_t Addr::sockaddrLen() const {
        const sockaddr* addr = reinterpret_cast<const sockaddr*>(storage_);
        return (addr->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    }

} // namespace pulse::net::udp