#pragma once

#include <netinet/in.h>
#include <string_view>
#include <array>
#include <functional>

namespace mdns {

class UnixSocket {
public:
    /// Socket descriptor
    struct SocketDP {
        int socket;
    };
    UnixSocket();
    ~UnixSocket();
    std::string_view hostname();

    // Unable to get interface addresses
    static constexpr int UNABLE_TO_GET_INTERFACE_ADDRESS = -1;
    static constexpr int MDNS_PORT = 5353;

    using AcceptInterface = std::function<bool(char* interfaceName, uint8_t interfaceIPAddr[16], size_t ipLen)>;
    using AddSocketCallback = std::function<void(SocketDP socketDp)>;

    /// Open client sockets
    ///
    /// When sending, each socket can only send to one network interface
    /// Thus we need to open one socket for each interface and address family
    ///
    /// \param port Port for sending
    /// \return Return the number of sockets
    int open_client_sockets(const AcceptInterface& predicate, const AddSocketCallback& addSocketCallback, int port = 0);

    /// Open service sockets on port MDNS_PORT
    ///
    /// When receiving, each socket can receive data from all network interfaces
    /// Thus we only need to open one socket for each address family
    ///
    /// \param sockets Application controlled memory for sockets
    /// \param max_sockets Maximum number of sockets
    /// \return Return the number of opened sockets
    std::array<SocketDP,2> open_service_sockets(bool IPv4, bool IPv6, uint16_t port = MDNS_PORT);

    int write();

    int readBlock();
private:
    std::array<char, 256> m_hostname_buffer{};
    std::string_view m_hostname;

    bool has_ipv6{};
    bool has_ipv4{};
    uint32_t service_address_ipv4{};
    uint8_t service_address_ipv6[16]{};
};

}
