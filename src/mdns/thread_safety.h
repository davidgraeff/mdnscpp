#pragma once

#include <mutex>

namespace mdns
{

class SingleThreadSafe
{
public:
    class Locker {};
    [[nodiscard]] Locker scopeLock() noexcept {
        return {};
    }
};

class MultiThreadSafe
{
    std::mutex mutex;
public:
    [[nodiscard]] std::scoped_lock<std::mutex> scopeLock() noexcept {
        return std::scoped_lock<std::mutex>(mutex);
    }
};

}
