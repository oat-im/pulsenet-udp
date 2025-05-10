#pragma once

#include <pulse/net/udp/socket_factory.h>
#include "win_socket.h"

namespace pulse::net::udp {

    class WindowsSocketFactory : public ISocketFactory {
    public:
        std::expected<std::unique_ptr<ISocket>, Error> listen(const Addr& bindAddr) override {
            return SocketWindows::Listen(bindAddr);
        }

        std::expected<std::unique_ptr<ISocket>, Error> dial(const Addr& remoteAddr) override {
            return SocketWindows::Dial(remoteAddr);
        }
    };

} // namespace pulse::net::udp