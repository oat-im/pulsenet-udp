<div align="center">
  <img src="https://pulsenet.dev/images/pulse-networking-social.png" alt="Pulse Networking" width="1200">
  <h1>pulse::net::udp</h1>
  <p><strong>Raw non-blocking UDP sockets with Go-style ergonomics, in modern C++23.</strong></p>
  <p>
    <a href="#features">Features</a> •
    <a href="#why">Why?</a> •
    <a href="#usage">Usage</a> •
    <a href="#build-requirements">Build Requirements</a> •
    <a href="#fetchcontent">FetchContent</a> •
    <a href="#platform-support">Platform Support</a> •
    <a href="#license">License</a>
  </p>
</div>

---

`pulse::net::udp` is a minimal, modern, cross-platform UDP socket layer written in pure C++23 — with sane error handling (`std::expected`), zero dependencies, and no framework bloat.

It does one thing well: **non-blocking UDP** across Unix and Windows.

## 🚀 Features

- ✅ Modern C++23 (`std::expected`, no exceptions)
- ✅ Non-blocking UDP sockets
- ✅ `Listen()` and `Dial()` like Go
- ✅ `send()` / `sendTo()` and `recvFrom()` with structured error handling
- ✅ Zero dependencies
- ✅ Cross-platform: Unix (Linux/macOS) and Windows (Winsock2)
- ✅ Dead simple integration

## 🧠 Why?

Because writing portable UDP in C++ is still a flaming trash heap:
- POSIX and Winsock APIs barely resemble each other
- Most libraries are bloated, legacy-bound, or layered abstractions on top of boost or libuv
- Nobody should still be writing socket() / bind() / recvfrom() directly in 2025

This library fixes that with a clean, modern API that doesn’t try to reinvent networking — just makes it suck less.

## 🧑‍💻 Usage

```cpp
#include <pulse/net/udp/udp.h> // For ISocket, Addr, ErrorCode
#include <pulse/net/udp/socket_factory.h> // For get_socket_factory()
#include <iostream>
#include <vector>

int main() {
    using namespace pulse::net::udp;

    auto serverAddrResult = Addr::Create("127.0.0.1", 9000);
    if (!serverAddrResult) {
        std::cerr << "Server Addr::Create failed: " << to_string(serverAddrResult.error()) << "\n";
        return 1;
    }
    auto& serverAddr = *serverAddrResult;

    ISocketFactory* factory = get_socket_factory();

    auto serverResult = factory->listen(serverAddr);
    if (!serverResult) {
        std::cerr << "Listen failed: " << to_string(serverResult.error()) << "\n";
        return 1;
    }
    auto& server = **serverResult; // Note: serverResult is expected<unique_ptr<ISocket>, ...>

    auto clientResult = factory->dial(serverAddr);
    if (!clientResult) {
        std::cerr << "Dial failed: " << to_string(clientResult.error()) << "\n";
        return 1;
    }
    auto& client = **clientResult;

    std::vector<uint8_t> message = {'h', 'e', 'l', 'l', 'o'};
    // Note: The original example uses client->send, which is fine for a dialed socket.
    // The example below assumes client is std::unique_ptr<ISocket>& client = *clientResult;
    if (auto res = client->send(message.data(), message.size()); !res) {
        std::cerr << "Send failed: " << to_string(res.error()) << "\n";
        return 1;
    }

    auto recvResult = server->recvFrom();
    if (!recvResult) {
        std::cerr << "Receive failed: " << to_string(recvResult.error()) << "\n";
        return 1;
    }

    const ReceivedPacket& packet = *recvResult;
    std::string msg(reinterpret_cast<const char*>(packet.data), packet.length);

    std::cout << "Received: " << msg << " from " << packet.addr.ip << ":" << packet.addr.port << "\n";
    return 0;
}
```

## 🏗 Build Requirements

* **C++23**
* **CMake ≥ 3.15**

If your compiler doesn’t support `std::expected`, upgrade. This is not a museum.

### ⚠️ Error Handling Philosophy

`pulse::net::udp` uses `std::expected` for all runtime operations. No exceptions are thrown during normal usage.

The **only exceptions** are constructors like `Addr(ip, port)`, where C++ gives no sane way to return an error. If construction fails due to invalid input (e.g. garbage IP address), you'll get a `std::invalid_argument`.

Everything else—`send()`, `recvFrom()`, `Dial()`, `Listen()`—uses `std::expected<T, ErrorCode>` so you can handle failures explicitly, without try/catch nonsense.

## 📦 FetchContent

You can pull in `pulse::net::udp` via `FetchContent` like this:

```cmake
include(FetchContent)

FetchContent_Declare(
  pulse_udp
  GIT_REPOSITORY https://git.pulsenet.dev/pulse/udp
  GIT_TAG        v1.0.0
)

FetchContent_MakeAvailable(pulse_udp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_link_libraries(your_target PRIVATE pulse::net::udp)
```

The actual target is aliased to `pulse::net::udp`, even though the library name is `pulsenet_udp`.

## 🪟 Platform Support

| Platform | Supported? | Notes                         |
| -------- | ---------- | ----------------------------- |
| Linux    | ✅          | `fcntl()` for non-blocking    |
| macOS    | ✅          | Same as above                 |
| Windows  | ✅          | Raw Winsock2 + `WSAStartup()` |

## ⚖️ License

**AGPLv3**. If that offends you, congratulations — it’s working as intended.

Want to use this in a proprietary product? [Buy a commercial license](https://pulsenet.dev/) or go write your own UDP stack.

## 🧨 Final Word

This isn’t boost. This isn’t some academic networking playground.

If you want a fast, lean UDP layer that doesn’t try to abstract away the world — and doesn’t get in your way when you're building serious low-latency systems — you're in the right place.

If you need a coroutine DSL, TLS tunnels, and a metrics dashboard, leave now.

## Version

**pulse::net::udp v1.0.0**