# Public domain mDNS/DNS-SD library for C++

This is a mDNS / service discovery and publish header-only C++17 library.
Optional parts of C++20 are used (concepts).

This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.

## Features

The library does DNS-SD discovery and service as well as single record mDNS query and response.

* Memory management is customizable via templates, with a pre-allocated buffer default implementation.
* Socket communication is customizable via templates. 
  Either existing BSD Socket API handlers can be used or the library manages sockets for you with the
  default implementation. The socket is initialized with multicast membership (including loopback) and set to non-blocking mode.


## Usage

The `examples/discovery.cpp` and `examples/publish.cpp` demonstrates the use of all features, including discovery, query and service response.
Build this repo with [CMake](https://cmake.org/download/) and find the example executables in your build directory.

### Initialize the library

First create a custom type with the features that you require:

```cpp
using Mdns = MdnsCpp<MdnsFixedSizeBuffer<3>, MdnsManagedSocket, MdnsThreadSafety>
```

The first template argument defines the memory management strategy.
You can chose between `MdnsFixedSizeBuffer` and `MdnsDynamicMemory`.

The second argument is either `MdnsManagedSocket` or a custom type that implements
the same methods and sub-types like `MdnsManagedSocket`.

### Discovery

To send a DNS-SD service discovery request use `mdns.discover()`.
This will send a single multicast packet (single question record for `_services._dns-sd._udp.local.`).

Use the returned `Mdns::QueryProcess` type and the method `receive() -> Optional<Mdns::QueryProcess>` to query for responses.
If there is another record available since the last call, it will be returned.

The second entry type will be one of `MDNS_ENTRYTYPE_ANSWER`, `MDNS_ENTRYTYPE_AUTHORITY` and `MDNS_ENTRYTYPE_ADDITIONAL`.

### Query

To send a mDNS query for a single record use `mdns.query(record : string_view)` with `record` = `_http._tcp.local.` for example.
This will send a single multicast query packet for the given record with a unique transaction id.

Use the returned `Mdns::QueryProcess` type and the method `receive() -> Optional<Mdns::QueryResult>` to query for responses (for the unique transaction id determined earlier).
If there is another record available since the last call, it will be returned.

The second entry type will be one of `MDNS_ENTRYTYPE_ANSWER`, `MDNS_ENTRYTYPE_AUTHORITY` and `MDNS_ENTRYTYPE_ADDITIONAL`.

### Service

If you use the default socket implementation, using this library in service / publish mode is straight-forward.

Call `mdns.publish(service)` to 

To listen for incoming DNS-SD requests and mDNS queries the socket should be opened on port `5353` (default) in call to the socket open/setup functions. Then call `mdns_socket_listen` either on notification of incoming data, or by setting blocking mode and calling `mdns_socket_listen` to block until data is available and parsed.

The entry type passed to the callback will be `MDNS_ENTRYTYPE_QUESTION` and record type `MDNS_RECORDTYPE_PTR`. Use the `mdns_record_parse_ptr` function to get the name string of the service record that was asked for.

If service record name is `_services._dns-sd._udp.local.` you should use `mdns_discovery_answer` to send the records of the services you provide (DNS-SD).

If the service record name is a service you provide, use `mdns_query_answer` to send the service details back in response to the query.

See the test executable implementation for more details on how to handle the parameters to the given functions.
