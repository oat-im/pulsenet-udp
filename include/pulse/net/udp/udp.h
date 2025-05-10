#pragma once

#include "udp_addr.h"
#include "error_code.h"
#include "socket_factory.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <utility>
#include <expected>

namespace pulse::net::udp {

    struct ReceivedPacket {
        uint8_t* data;
        size_t size; // Size of the received data
        size_t capacity; // Size of the allocated buffer
        Addr addr;
    };

    class ISocket {
    public:
        virtual ~ISocket() = default;

        // Send a packet to the given address
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<void, Error> sendTo(const Addr& addr, const uint8_t* data, size_t length) = 0;

        // Send a packet to the connected address
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<void, Error> send(const uint8_t* data, size_t length) = 0;

        /// Receives a packet. The returned `data` pointer is valid only until the next recvFrom() call on the same thread.
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<ReceivedPacket, Error> recvFrom() = 0;

        /// Receives a packet from a connected address using a given buffer.
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<ReceivedPacket, Error> recvFrom(ReceivedPacket&& packet) = 0;

        // Returns underlying socket fd/handle if needed
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<int, Error> getHandle() const = 0;

        // Close the socket
        virtual void close() = 0;
    };

}
