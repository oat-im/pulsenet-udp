#pragma once

#include <string>
#include <string_view>
#include <exception>
#include <expected>

namespace pulse::net::udp {

    enum class ErrorCode {
        InvalidAddress = 1,
        BindFailed,
        ConnectFailed,
        RecvFailed,
        SendFailed,
        Timeout,
        Closed,
        WouldBlock,
        InvalidSocket,
        ConnectionReset,
        PartialSend,
        UnsupportedAddressFamily,
        SocketCreateFailed,
        SocketConfigFailed,
        WSAStartupFailed,
        Unknown = 9999
    };
    inline constexpr const char* error_to_string(ErrorCode code) noexcept;

    // Error is POD and we therefore allow direct construction because we ensure that the constructor is constexpr.
    struct Error {
    public:
        ErrorCode code;
        std::string message;
    
        constexpr Error(ErrorCode c) noexcept : code(c) {}
        Error(ErrorCode c, const std::exception& e) noexcept : code(c), message(concat(c, e.what())) {}
        Error(ErrorCode c, std::string msg) noexcept : code(c), message(concat(c, std::move(msg))) {}
    
        [[nodiscard("You're ignoring an error message. Don't do that.")]]
        std::string_view what() const noexcept {
            if (!message.empty()) {
                return message;
            } else {
                return error_to_string(code);
            }
        }
    
        [[nodiscard("If you're going to use this, you should probably do something with it.")]]
        ErrorCode code_value() const noexcept { return code; }
    
        bool operator==(ErrorCode other) const noexcept { return code == other; }
        bool operator!=(ErrorCode other) const noexcept { return code != other; }

    private:
        static std::string concat(ErrorCode code, const std::string& msg) noexcept {
            try {
                return std::string(error_to_string(code)) + ": " + msg;
            } catch (...) {
                return std::string(error_to_string(code)); // fallback if even std::string throws (rare but technically possible)
            }
        }
    };

    inline std::string to_string(const Error& error) noexcept {
        return std::string(error.what()); // Always return a copy to be safe
    }

    template <class T>
    inline std::string to_string(const std::expected<T, Error>& error) noexcept {
        return to_string(error.error());
    }

    template <class... Args>
    inline std::unexpected<Error> make_unexpected(Args&&... args) noexcept {
        static_assert(std::is_constructible_v<Error, Args...>, 
                    "Invalid arguments for Error constructor.");
        return std::unexpected<Error>(std::in_place, std::forward<Args>(args)...);
    }

    inline constexpr const char* error_to_string(ErrorCode code) noexcept {
        switch (code) {
            case ErrorCode::InvalidAddress: return "Invalid address";
            case ErrorCode::BindFailed: return "Bind failed";
            case ErrorCode::ConnectFailed: return "Connect failed";
            case ErrorCode::RecvFailed: return "Receive failed";
            case ErrorCode::SendFailed: return "Send failed";
            case ErrorCode::Timeout: return "Timeout";
            case ErrorCode::Closed: return "Socket closed";
            case ErrorCode::WouldBlock: return "Operation would block";
            case ErrorCode::InvalidSocket: return "Invalid socket";
            case ErrorCode::ConnectionReset: return "Connection reset";
            case ErrorCode::PartialSend: return "Partial send";
            case ErrorCode::UnsupportedAddressFamily: return "Unsupported address family";
            case ErrorCode::SocketCreateFailed: return "Socket creation failed";
            case ErrorCode::SocketConfigFailed: return "Socket configuration failed";
            case ErrorCode::WSAStartupFailed: return "WSAStartup failed";
            default: return "Unknown error";
        }
    }

} // namespace pulse::net::udp