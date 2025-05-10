#include <pulse/net/udp/udp.h>
#include <winsock2.h>

namespace pulse::net::udp {

    class SocketWindows : public ISocket {
    public:
        SocketWindows(SOCKET sock);
        ~SocketWindows();

        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<void, Error> sendTo(const Addr& addr, const uint8_t* data, size_t length) override;
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<void, Error> send(const uint8_t* data, size_t length) override;
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<ReceivedPacket, Error> recvFrom() override;
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<ReceivedPacket, Error> recvFrom(ReceivedPacket&& packet) override;
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::expected<int, Error> getHandle() const override;

        void close() override;
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<Addr, Error> DecodeAddr(const sockaddr* addr);
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<std::unique_ptr<ISocket>, Error> Listen(const Addr& bindAddr);
        
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        static std::expected<std::unique_ptr<ISocket>, Error> Dial(const Addr& remoteAddr);
        
    private:
        SOCKET sock_;
        
    };

} // namespace pulse::net::udp