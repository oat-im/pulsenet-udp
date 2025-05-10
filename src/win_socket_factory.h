#pragma once

#include <pulse/net/udp/socket_factory.h>
#include "win_socket.h"

namespace pulse::net::udp {

    class WindowsSocketFactory : public ISocketFactory {
    public:
        std::expected<std::unique_ptr<ISocket>, Error> listen(const Addr& bind_addr) override {
            return SocketWindows::Listen(bind_addr);
        }

        std::expected<std::unique_ptr<ISocket>, Error> dial(const Addr& remote_addr) override {
            return SocketWindows::Dial(remote_addr);
        }
    };

} // namespace pulse::net::udp