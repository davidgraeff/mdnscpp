#pragma once

/// Concepts in this library are entirely optionally and help to develop own implementations
/// for memory management, thread safety and the socket layer with rich compiler error messages.

#include "buffers.h"
#include "thread_safety.h"

#include <string_view>

#if __cpp_concepts
#include <concepts>
#endif

namespace mdns
{

#if __cpp_concepts

template <class T>
concept MemoryManagerType =
    requires (T x, size_t i) {
        { x.allocate(i) } -> std::convertible_to<size_t> ;
        { x.deallocate(i) } -> std::convertible_to<size_t> ;
    };

template<class T>
concept AcceptInterfaceConcept = std::invocable<bool(char*, uint8_t[16], size_t), char*, uint8_t[16], size_t>;

template <class T, class SocketDP>
concept AddSocketCallbackConcept = std::invocable<void(SocketDP), SocketDP>;

template <class T>
concept SocketLayerType = std::is_default_constructible_v<T> &&
requires (T x, int size, int port, typename T::SocketDP* socketDp, typename T::AcceptInterface pre, typename T::AddSocketCallback addSocketCallback) {
    requires AcceptInterfaceConcept<typename T::AcceptInterface>;
    requires AddSocketCallbackConcept<typename T::AddSocketCallback, typename T::SocketDP>;
    { x.hostname() } -> std::same_as<std::string_view>;
    { x.open_service_sockets(socketDp, size) } -> std::convertible_to<std::array<typename T::SocketDP,2>>;
    { x.open_client_sockets(pre, addSocketCallback, port) } -> std::convertible_to<int>;
};


template <class T>
concept ThreadSafetyScopeType = std::destructible<T>;

template <class T>
concept ThreadSafetyManagerType =
requires (T x) {
    { x.scopeLock() } -> ThreadSafetyScopeType ;
};

#else
// Fallback; just use c++ template class keyword

    #define MemoryManagerType class
    #define SocketLayerType class
    #define ThreadSafetyManagerType class
#endif

}
