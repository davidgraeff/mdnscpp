#include "socket_unix.h"

#include <cstring>

#include <fcntl.h>
#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define strncasecmp _strnicmp
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#endif

namespace {

void closeSocket(int sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int mdns_socket_setup_ipv4(int sock, struct sockaddr_in* saddr) {
    unsigned char ttl = 1;
    unsigned char loopback = 1;
    unsigned int reuseaddr = 1;
    ip_mreq req{};

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuseaddr, sizeof(reuseaddr));
#endif
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, sizeof(ttl));
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loopback, sizeof(loopback));

    memset(&req, 0, sizeof(req));
    req.imr_multiaddr.s_addr = htonl((((uint32_t)224U) << 24U) | ((uint32_t)251U));
    if (saddr)
        req.imr_interface = saddr->sin_addr;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&req, sizeof(req)))
        return -1;

    sockaddr_in sock_addr{};
    if (!saddr) {
        saddr = &sock_addr;
        memset(saddr, 0, sizeof(struct sockaddr_in));
        saddr->sin_family = AF_INET;
        saddr->sin_addr.s_addr = INADDR_ANY;
#ifdef __APPLE__
        saddr->sin_len = sizeof(struct sockaddr_in);
#endif
    } else {
        setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&saddr->sin_addr,
                   sizeof(saddr->sin_addr));
#ifndef _WIN32
        saddr->sin_addr.s_addr = INADDR_ANY;
#endif
    }

    if (bind(sock, (struct sockaddr*)saddr, sizeof(struct sockaddr_in)))
        return -1;

#ifdef _WIN32
    unsigned long param = 1;
	ioctlsocket(sock, FIONBIO, &param);
#else
    const int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    return 0;
}

int mdns_socket_setup_ipv6(int sock, struct sockaddr_in6* saddr) {
    int hops = 1;
    unsigned int loopback = 1;
    unsigned int reuseaddr = 1;
    ipv6_mreq req{};

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuseaddr, sizeof(reuseaddr));
#endif
    setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char*)&hops, sizeof(hops));
    setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (const char*)&loopback, sizeof(loopback));

    memset(&req, 0, sizeof(req));
    req.ipv6mr_multiaddr.s6_addr[0] = 0xFF;
    req.ipv6mr_multiaddr.s6_addr[1] = 0x02;
    req.ipv6mr_multiaddr.s6_addr[15] = 0xFB;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*)&req, sizeof(req)))
        return -1;

    sockaddr_in6 sock_addr{};
    if (!saddr) {
        saddr = &sock_addr;
        memset(saddr, 0, sizeof(struct sockaddr_in6));
        saddr->sin6_family = AF_INET6;
        saddr->sin6_addr = in6addr_any;
#ifdef __APPLE__
        saddr->sin6_len = sizeof(struct sockaddr_in6);
#endif
    } else {
        unsigned int ifindex = 0;
        setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (const char*)&ifindex, sizeof(ifindex));
#ifndef _WIN32
        saddr->sin6_addr = in6addr_any;
#endif
    }

    if (bind(sock, (struct sockaddr*)saddr, sizeof(struct sockaddr_in6)))
        return -1;

#ifdef _WIN32
    unsigned long param = 1;
	ioctlsocket(sock, FIONBIO, &param);
#else
    const int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    return 0;
}

int open_socket(sockaddr_in* saddr) {
    int sock = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        return -1;
    if (mdns_socket_setup_ipv4(sock, saddr)) {
        closeSocket(sock);
        return -1;
    }
    return sock;
}

int open_socket(sockaddr_in6* saddr) {
    int sock = (int)socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        return -1;
    if (mdns_socket_setup_ipv6(sock, saddr)) {
        closeSocket(sock);
        return -1;
    }
    return sock;
}

}

using namespace mdns;

std::string_view UnixSocket::hostname() {
    return m_hostname;
}

UnixSocket::UnixSocket() {
#ifdef _WIN32

    WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	if (WSAStartup(versionWanted, &wsaData)) {
		printf("Failed to initialize WinSock\n");
		return -1;
	}

	DWORD hostname_size = (DWORD)m_hostname_buffer.size();
	if (GetComputerNameA(m_hostname_buffer.data(), &hostname_size))
		hostname = m_hostname_buffer.data();
	else
	    m_hostname = "dummy-host";

#else

    if (gethostname(m_hostname_buffer.data(), m_hostname_buffer.size()) == 0)
        m_hostname = m_hostname_buffer.data();
    else
        m_hostname = "dummy-host";

#endif
}

UnixSocket::~UnixSocket() {
#ifdef _WIN32
    WSACleanup();
#endif
}

int UnixSocket::open_client_sockets(const AcceptInterface& predicate, const AddSocketCallback& addSocketCallback, int port) {
    // When sending, each socket can only send to one network interface
    // Thus we need to open one socket for each interface and address family
    int num_sockets = 0;

#ifdef _WIN32

    IP_ADAPTER_ADDRESSES* adapter_address = 0;
	ULONG address_size = 8000;
	unsigned int ret;
	unsigned int num_retries = 4;
	do {
		adapter_address = malloc(address_size);
		ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST, 0,
		                           adapter_address, &address_size);
		if (ret == ERROR_BUFFER_OVERFLOW) {
			free(adapter_address);
			adapter_address = 0;
		} else {
			break;
		}
	} while (num_retries-- > 0);

	if (!adapter_address || (ret != NO_ERROR)) {
		free(adapter_address);
		return UNABLE_TO_GET_INTERFACE_ADDRESS;
	}

	bool first_ipv4 = true;
	bool first_ipv6 = true;
	for (PIP_ADAPTER_ADDRESSES adapter = adapter_address; adapter; adapter = adapter->Next) {
		if (adapter->TunnelType == TUNNEL_TYPE_TEREDO)
			continue;
		if (adapter->OperStatus != IfOperStatusUp)
			continue;

		for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast;
		     unicast = unicast->Next) {
			if (unicast->Address.lpSockaddr->sa_family == AF_INET) {
				struct sockaddr_in* saddr = (struct sockaddr_in*)unicast->Address.lpSockaddr;
				if ((saddr->sin_addr.S_un.S_un_b.s_b1 == 127) &&
				    (saddr->sin_addr.S_un.S_un_b.s_b2 == 0) &&
				    (saddr->sin_addr.S_un.S_un_b.s_b3 == 0) &&
				    (saddr->sin_addr.S_un.S_un_b.s_b4 == 1))
				    continue;

                if (first_ipv4) {
                    service_address_ipv4 = saddr->sin_addr.S_un.S_addr;
                    first_ipv4 = false;
                }
                has_ipv4 = true;
                if (num_sockets < max_sockets && predicate()) {
                    saddr->sin_port = htons((unsigned short)port);
                    int sock = mdns_socket_open_ipv4(saddr);
                    if (sock >= 0) {
                        sockets[num_sockets++].socket = sock;
                    }
                }
			} else if (unicast->Address.lpSockaddr->sa_family == AF_INET6) {
				struct sockaddr_in6* saddr = (struct sockaddr_in6*)unicast->Address.lpSockaddr;
				static const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0,
				                                          0, 0, 0, 0, 0, 0, 0, 1};
				static const unsigned char localhost_mapped[] = {0, 0, 0,    0,    0,    0, 0, 0,
				                                                 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
				if ((unicast->DadState != NldsPreferred) ||
				    memcmp(saddr->sin6_addr.s6_addr, localhost, 16) == 0 ||
				    memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16) == 0)
				    continue;

                if (first_ipv6) {
                    memcpy(service_address_ipv6, &saddr->sin6_addr, 16);
                    first_ipv6 = false;
                }
                has_ipv6 = true;
                if (num_sockets < max_sockets) {
                    saddr->sin6_port = htons((unsigned short)port);
                    int sock = mdns_socket_open_ipv6(saddr);
                    if (sock >= 0) {
                        sockets[num_sockets++].socket = sock;
                    }
                }
			}
		}
	}

	free(adapter_address);

#else

    ifaddrs* ifaddr = nullptr;

    if (getifaddrs(&ifaddr) < 0)
        return UNABLE_TO_GET_INTERFACE_ADDRESS;

    bool first_ipv4 = true;
    bool first_ipv6 = true;
    for (ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            auto* saddr = (struct sockaddr_in*)ifa->ifa_addr;
            if (saddr->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                continue;
            if (first_ipv4) {
                service_address_ipv4 = saddr->sin_addr.s_addr;
                first_ipv4 = false;
            }
            has_ipv4 = true;
            saddr->sin_port = htons(port);
            uint8_t interfaceIPAddr[16];
            memcpy(interfaceIPAddr, &saddr->sin_addr.s_addr, 4);
            if (predicate(ifa->ifa_name, interfaceIPAddr, 4)) {
                int sock = open_socket(saddr);
                if (sock >= 0) {
                    addSocketCallback(SocketDP{sock});
                }
            }
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            auto* saddr = (struct sockaddr_in6*)ifa->ifa_addr;
            static const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                                      0, 0, 0, 0, 0, 0, 0, 1};
            static const unsigned char localhost_mapped[] = {0, 0, 0,    0,    0,    0, 0, 0,
                                                             0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};

            if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) == 0 ||
                memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16) == 0)
                continue;

            if (first_ipv6) {
                memcpy(service_address_ipv6, &saddr->sin6_addr, 16);
                first_ipv6 = false;
            }
            has_ipv6 = true;
            saddr->sin6_port = htons(port);

            if (predicate(ifa->ifa_name, saddr->sin6_addr.s6_addr, 16)) {
                int sock = open_socket(saddr);
                if (sock >= 0) {
                    addSocketCallback(SocketDP{sock});
                }
            }
        }
    }

    freeifaddrs(ifaddr);

#endif

    return num_sockets;
}

/*
char buffer[128];
mdns_string_t addr = ipv4_address_to_string(buffer, sizeof(buffer), saddr,
                                            sizeof(struct sockaddr_in));
printf("Local IPv4 address: %.*s\n", MDNS_STRING_FORMAT(addr));
char buffer[128];
std::string_view addr = ipv6_address_to_string(buffer, sizeof(buffer), saddr,
                                            sizeof(struct sockaddr_in6));
printf("Local IPv6 address: %.*s\n", MDNS_STRING_FORMAT(addr));
 */

std::array<UnixSocket::SocketDP,2> UnixSocket::open_service_sockets(bool IPv4, bool IPv6, uint16_t port) {
    // Call the client socket function to enumerate and get local addresses,
    // but not open the actual sockets
    open_client_sockets([](char* name, uint8_t ip[16], size_t ipLen){return false;},AddSocketCallback{});

    std::array<UnixSocket::SocketDP,2> sockets{};

    if (IPv4) {
        sockaddr_in sock_addr{};
        sock_addr.sin_family = AF_INET;
#ifdef _WIN32
        sock_addr.sin_addr = in4addr_any;
#else
        sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif
        sock_addr.sin_port = htons(port);
#ifdef __APPLE__
        sock_addr.sin_len = sizeof(struct sockaddr_in);
#endif
        int sock = open_socket(&sock_addr);
        if (sock >= 0)
            sockets[0].socket = sock;
    } else {
        sockets[0].socket = -1;
    }

    if (IPv6) {
        sockaddr_in6 sock_addr{};
        sock_addr.sin6_family = AF_INET6;
        sock_addr.sin6_addr = in6addr_any;
        sock_addr.sin6_port = htons(port);
#ifdef __APPLE__
        sock_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
        int sock = open_socket(&sock_addr);
        if (sock >= 0)
            sockets[1].socket = sock;
    } else {
        sockets[1].socket = -1;
    }

    return sockets;
}
