#include "pulse/net/udp/udp_addr.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <cstring>

namespace pulse::net::udp {

    const char* Addr::kAnyIPv4 = "0.0.0.0";
    const char* Addr::kAnyIPv6 = "::";

    Addr::Addr(const std::string& ipStr, uint16_t port)
        : ip(ipStr), port(port)
    {
        if (inet_pton(AF_INET, ipStr.c_str(), &reinterpret_cast<sockaddr_in*>(storage_)->sin_addr) == 1) {
            auto* addr = reinterpret_cast<sockaddr_in*>(storage_);
            addr->sin_family = AF_INET;
            addr->sin_port = htons(port);
        } else if (inet_pton(AF_INET6, ipStr.c_str(), &reinterpret_cast<sockaddr_in6*>(storage_)->sin6_addr) == 1) {
            auto* addr6 = reinterpret_cast<sockaddr_in6*>(storage_);
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = htons(port);
        } else {
            throw std::invalid_argument("Invalid IP address: " + ipStr);
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