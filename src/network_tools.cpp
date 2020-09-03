#include "network_tools.h"

#ifdef _WIN32
#include <iphlpapi.h>
#else

#include <netdb.h>
#include <ifaddrs.h>

#endif

#include <stdio.h>

std::string_view ip_address_to_string(char *buffer, size_t capacity, const struct sockaddr *addr, size_t addrlen) {
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo(addr, (socklen_t) addrlen, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    size_t len = 0;
    if (ret == 0) {
        if (addr->sa_family == AF_INET6) {
            if (((const struct sockaddr_in6 *)addr)->sin6_port != 0)
                len = snprintf(buffer, capacity, "[%s]:%s", host, service);
            else
                len = snprintf(buffer, capacity, "%s", host);
        } else {
            if (((const struct sockaddr_in *)addr)->sin_port != 0)
                len = snprintf(buffer, capacity, "%s:%s", host, service);
            else
                len = snprintf(buffer, capacity, "%s", host);
        }
    }
    if (len >= capacity)
        len = capacity - 1;

    return std::string_view{buffer, len};
}
