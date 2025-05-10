#pragma once

#include <pulse/net/udp/error_code.h>

#include <string>
#include <cstdint>
#include <expected>
#include <functional>
#include <stdexcept>

namespace pulse::net::udp {

    class Addr {
    public:
        // We need to be able to support default construction.
        Addr() = default;
        std::string ip;
        uint16_t port;

        // Internal socket address access
        [[nodiscard("Why are you ignoring this?")]]
        const void* sockaddrData() const;

        [[nodiscard("Why did you ask for this and then ignore it?")]]
        size_t sockaddrLen() const;

        // Let's create a constant AnyIPv4 and AnyIPv6 address const char * for convenience
        static const char * kAnyIPv4;
        static const char * kAnyIPv6;

        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<Addr, Error> Create(const std::string& ip, uint16_t port) {
            try {
                return Addr(ip, port);
            } catch (const std::invalid_argument& err) {
                return std::unexpected(Error(ErrorCode::InvalidAddress, err));
            } catch (...) {
                return std::unexpected(Error(ErrorCode::Unknown, "Unknown error occurred while creating Addr"));
            }
        }

    private:
        alignas(16) char storage_[128]{}; // big enough for sockaddr_in6
        
        Addr(const std::string& ip, uint16_t port);
    };

    inline bool operator==(const Addr& lhs, const Addr& rhs) {
        return lhs.ip == rhs.ip && lhs.port == rhs.port;
    }

} // namespace pulse::net::udp

namespace std {
    template <>
    struct hash<pulse::net::udp::Addr> {
        size_t operator()(const pulse::net::udp::Addr& a) const noexcept {
            size_t h1 = std::hash<std::string>{}(a.ip);
            size_t h2 = std::hash<uint16_t>{}(a.port);
            return h1 ^ (h2 << 1);
        }
    };
}
