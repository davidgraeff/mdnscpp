#pragma once

#include <memory>
#include "buffers.h"
#include "thread_safety.h"
#include "socket_unix.h"
#include "cpp_concepts.h"

#include <cstdio>
#include <cstring>
namespace mdns
{

template<MemoryManagerType MemoryManager, SocketLayerType SocketLayer, ThreadSafetyManagerType ThreadSafetyManager>
class Mdns
{
public:
    ///
    int service_mdns(const char* hostname, const char* service, int service_port);

    /// Query for one specific service
    ///
    /// This is a blocking call.
    /// \param service The service to query for. For example "_test-mdns._tcp.local."
    /// \return
    int query(std::string_view service);

    /// Service discovery
    int discover();
private:
    SocketLayer sockets;
};

using MdnsDefault = Mdns<FixedSizeBuffer<5>,UnixSocket,SingleThreadSafe>;
using MdnsMultThread = Mdns<FixedSizeBuffer<5>,UnixSocket,MultiThreadSafe>;

/// Implementation ///


template<MemoryManagerType MemoryManager, SocketLayerType SocketLayer, ThreadSafetyManagerType ThreadSafetyManager>
int Mdns<MemoryManager, SocketLayer, ThreadSafetyManager>::discover() {
    typename SocketLayer::SocketDP socketsBuffer[10];
    int socketCounter = 0;

    sockets.open_client_sockets([](char* interfaceName, uint8_t interfaceIPAddr[16], size_t ipLen){return true;},
                                                  [&socketsBuffer,&socketCounter](typename SocketLayer::SocketDP socketDp) {
        socketsBuffer[socketCounter++] = socketDp;
        }, 0);

    if (socketCounter <= 0) {
        printf("Failed to open any client sockets\n");
        return -1;
    }
    printf("Opened %d socket%s for DNS-SD\n", socketCounter, socketCounter ? "s" : "");

    printf("Sending DNS-SD discovery\n");
    for (int isock = 0; isock < socketCounter; ++isock) {
        if (mdns_discovery_send(socketsBuffer[isock]))
            printf("Failed to send DNS-DS discovery: %s\n", strerror(errno));
    }

    size_t capacity = 2048;
    void* buffer = malloc(capacity);
    void* user_data = nullptr;
    size_t records;

    // This is a simple implementation that loops for 5 seconds or as long as we get replies
    int res;
    printf("Reading DNS-SD replies\n");
    do {
        timeval timeout{};
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int isock = 0; isock < num_sockets; ++isock) {
            if (socketsBuffer[isock] >= nfds)
                nfds = socketsBuffer[isock] + 1;
            FD_SET(socketsBuffer[isock], &readfs);
        }

        records = 0;
        res = select(nfds, &readfs, 0, 0, &timeout);
        if (res > 0) {
            for (int isock = 0; isock < num_sockets; ++isock) {
                if (FD_ISSET(socketsBuffer[isock], &readfs)) {
                    records += mdns_discovery_recv(socketsBuffer[isock], buffer, capacity, query_callback, user_data);
                }
            }
        }
    } while (res > 0);

    free(buffer);

    for (int isock = 0; isock < num_sockets; ++isock)
        mdns_socket_close(socketsBuffer[isock]);
    printf("Closed socket%s\n", num_sockets ? "s" : "");

    return 0;
}

template<MemoryManagerType MemoryManager, SocketLayerType SocketLayer, ThreadSafetyManagerType ThreadSafetyManager>
int Mdns<MemoryManager, SocketLayer, ThreadSafetyManager>::query(std::string_view service) {
    typename SocketLayer::SocketDP socketsBuffer[32];
    int query_id[32];
    int num_sockets = sockets.open_client_sockets(socketsBuffer, sizeof(socketsBuffer) / sizeof(socketsBuffer[0]), 0);
    if (num_sockets <= 0) {
        printf("Failed to open any client sockets\n");
        return -1;
    }
    printf("Opened %d socket%s for mDNS query\n", num_sockets, num_sockets ? "s" : "");

    size_t capacity = 2048;
    void* buffer = malloc(capacity);
    void* user_data = 0;
    size_t records;

    printf("Sending mDNS query: %s\n", service.data());
    for (int isock = 0; isock < num_sockets; ++isock) {
        query_id[isock] = mdns_query_send(socketsBuffer[isock], MDNS_RECORDTYPE_PTR, service.data(), service.size(), buffer, capacity, 0);
        if (query_id[isock] < 0)
            printf("Failed to send mDNS query: %s\n", strerror(errno));
    }

    // This is a simple implementation that loops for 5 seconds or as long as we get replies
    int res;
    printf("Reading mDNS query replies\n");
    do {
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int isock = 0; isock < num_sockets; ++isock) {
            if (socketsBuffer[isock] >= nfds)
                nfds = socketsBuffer[isock] + 1;
            FD_SET(socketsBuffer[isock], &readfs);
        }

        records = 0;
        res = select(nfds, &readfs, nullptr, nullptr, &timeout);
        if (res > 0) {
            for (int isock = 0; isock < num_sockets; ++isock) {
                if (FD_ISSET(socketsBuffer[isock], &readfs)) {
                    records += mdns_query_recv(socketsBuffer[isock], buffer, capacity, query_callback,
                                               user_data, query_id[isock]);
                }
                FD_SET(socketsBuffer[isock], &readfs);
            }
        }
    } while (res > 0);

    free(buffer);

    for (int isock = 0; isock < num_sockets; ++isock)
        mdns_socket_close(socketsBuffer[isock]);
    printf("Closed socket%s\n", num_sockets ? "s" : "");

    return 0;
}

template<MemoryManagerType MemoryManager, SocketLayerType SocketLayer, ThreadSafetyManagerType ThreadSafetyManager>
int Mdns<MemoryManager, SocketLayer, ThreadSafetyManager>::service_mdns(const char* hostname, const char* service, int service_port) {
    typename SocketLayer::SocketDP socketsBuffer[32];
    int num_sockets = sockets.open_service_sockets(socketsBuffer, sizeof(socketsBuffer) / sizeof(socketsBuffer[0]));
    if (num_sockets <= 0) {
        printf("Failed to open any client sockets\n");
        return -1;
    }
    printf("Opened %d socket%s for mDNS service\n", num_sockets, num_sockets ? "s" : "");

    printf("Service mDNS: %s:%d\n", service, service_port);
    printf("Hostname: %s\n", hostname);

    size_t capacity = 2048;
    void* buffer = malloc(capacity);

    service_record_t service_record;
    service_record.service = service;
    service_record.hostname = hostname;
    service_record.address_ipv4 = has_ipv4 ? service_address_ipv4 : 0;
    service_record.address_ipv6 = has_ipv6 ? service_address_ipv6 : 0;
    service_record.port = service_port;

    // This is a crude implementation that checks for incoming queries
    while (true) {
        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int isock = 0; isock < num_sockets; ++isock) {
            if (socketsBuffer[isock] >= nfds)
                nfds = socketsBuffer[isock] + 1;
            FD_SET(socketsBuffer[isock], &readfs);
        }

        if (select(nfds, &readfs, nullptr, nullptr, nullptr) >= 0) {
            for (int isock = 0; isock < num_sockets; ++isock) {
                if (FD_ISSET(socketsBuffer[isock], &readfs)) {
                    mdns_socket_listen(socketsBuffer[isock], buffer, capacity, service_callback, &service_record);
                }
                FD_SET(socketsBuffer[isock], &readfs);
            }
        } else {
            break;
        }
    }

    free(buffer);

    for (int isock = 0; isock < num_sockets; ++isock)
        mdns_socket_close(socketsBuffer[isock]);
    printf("Closed socket%s\n", num_sockets ? "s" : "");

    return 0;
}

}
