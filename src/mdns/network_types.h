#pragma once
#include <cstdint>

enum mdns_record_type_t {
    MDNS_RECORDTYPE_IGNORE = 0,
    // Address
    MDNS_RECORDTYPE_A = 1,
    // Domain Name pointer
    MDNS_RECORDTYPE_PTR = 12,
    // Arbitrary text string
    MDNS_RECORDTYPE_TXT = 16,
    // IP6 Address [Thomson]
    MDNS_RECORDTYPE_AAAA = 28,
    // Server Selection [RFC2782]
    MDNS_RECORDTYPE_SRV = 33
};

enum mdns_entry_type_t {
    MDNS_ENTRYTYPE_QUESTION = 0,
    MDNS_ENTRYTYPE_ANSWER = 1,
    MDNS_ENTRYTYPE_AUTHORITY = 2,
    MDNS_ENTRYTYPE_ADDITIONAL = 3
};

enum mdns_class_t { MDNS_CLASS_IN = 1 };

struct mdns_header_t {
    uint16_t query_id;
    uint16_t flags;
    uint16_t questions;
    uint16_t answer_rrs;
    uint16_t authority_rrs;
    uint16_t additional_rrs;
};
