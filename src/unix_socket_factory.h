#pragma once

#include <pulse/net/udp/socket_factory.h>
#include "unix_socket.h"

namespace pulse::net::udp {

    class UnixSocketFactory : public ISocketFactory {
    public:
        std::expected<std::unique_ptr<ISocket>, Error> listen(const Addr& bindAddr) override {
            return SocketUnix::Listen(bindAddr);
        }

        std::expected<std::unique_ptr<ISocket>, Error> dial(const Addr& remoteAddr) override {
            return SocketUnix::Dial(remoteAddr);
        }
    };

} // namespace pulse::net::udp