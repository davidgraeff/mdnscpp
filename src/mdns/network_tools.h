#pragma once

#include <string_view>

std::string_view ip_address_to_string(char* buffer, size_t capacity, const struct sockaddr* addr, size_t addrlen);