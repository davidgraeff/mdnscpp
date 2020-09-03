#pragma once

#include <cstddef>

namespace mdns
{

template<int NUM>
class FixedSizeBuffer
{
public:
    size_t allocate(size_t) {return 0;}
    size_t deallocate(size_t) {return 0;}
};

}
