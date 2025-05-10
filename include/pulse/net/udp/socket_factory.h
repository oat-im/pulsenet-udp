#pragma once

#include "udp_addr.h"
#include "error_code.h"
#include <memory>
#include <expected>

namespace pulse::net::udp {

    class ISocket;
    class ISocketFactory {
    public:
        virtual ~ISocketFactory() = default;
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<std::unique_ptr<ISocket>, Error> listen(const Addr& bindAddr) = 0;

        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        virtual std::expected<std::unique_ptr<ISocket>, Error> dial(const Addr& remoteAddr) = 0;
    };

    // Free function for user convenience
    [[nodiscard("Don't ask for a socket factory and then ignore it.")]]
    ISocketFactory * get_socket_factory();
    void set_socket_factory(ISocketFactory * factory);
    void reset_socket_factory(); // Resets to default factory (if any)

} // namespace pulse::net::udp
