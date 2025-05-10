#pragma once

#include <pulse/net/udp/socket_factory.h>
#include "unix_socket.h"

namespace pulse::net::udp {

    class UnixSocketFactory : public ISocketFactory {
    public:
        std::expected<std::unique_ptr<ISocket>, Error> listen(const Addr& bind_addr) override {
            return SocketUnix::Listen(bind_addr);
        }

        std::expected<std::unique_ptr<ISocket>, Error> dial(const Addr& remote_addr) override {
            return SocketUnix::Dial(remote_addr);
        }
    };

} // namespace pulse::net::udp