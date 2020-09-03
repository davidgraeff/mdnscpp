cmake_minimum_required(VERSION 3.16)
project(mdnscpp)

add_library(mdnscpp src/mdns.cpp src/socket_unix.cpp src/mdns_old.cpp)
target_include_directories(mdnscpp PUBLIC src/mdns)
set_property(TARGET mdnscpp PROPERTY CXX_STANDARD 20)

option(BUILD_RESOLVER "Build example resolver binary" ON)
option(BUILD_PUBLISHER "Build example publisher binary" ON)

if(BUILD_RESOLVER)
    add_executable(mdns_responder examples/resolver.cpp)
    target_link_libraries(mdns_responder PRIVATE mdnscpp)
    set_property(TARGET mdns_responder PROPERTY CXX_STANDARD 20)
endif()