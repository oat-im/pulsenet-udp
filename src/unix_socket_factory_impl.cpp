#include <pulse/net/udp/socket_factory.h>
#include <pulse/net/udp/udp.h>
#include <pulse/net/udp/error_code.h>
#include <pulse/net/udp/udp_addr.h>

#include "unix_socket_factory.h"

namespace pulse::net::udp {

    namespace {
        ISocketFactory * current_factory = nullptr;
        UnixSocketFactory default_socket_factory;
    }

    ISocketFactory * get_socket_factory() {
        if (!current_factory) {
            current_factory = &default_socket_factory;
        }
        return current_factory;
    }

    void set_socket_factory(ISocketFactory * factory)
    {
        current_factory = factory;
    }
    
    void reset_socket_factory()
    {
        current_factory = nullptr;
    }
    
} // namespace pulse::net::udp