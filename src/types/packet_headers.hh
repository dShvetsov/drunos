#pragma once

#include <cstring>

#include <boost/endian/arithmetic.hpp>
#include <boost/exception/error_info.hpp>

namespace runos {
// TODO: namespace hdr {

struct ethernet_hdr {
    boost::endian::big_uint48_t dst;
    boost::endian::big_uint48_t src;
    boost::endian::big_uint16_t type;

    size_t header_length() const
    { return sizeof(*this); }
};
static_assert(sizeof(ethernet_hdr) == 14, "");

struct dot1q_hdr {
    boost::endian::big_uint48_t dst;
    boost::endian::big_uint48_t src;
    boost::endian::big_uint16_t tpid;
    union {
        boost::endian::big_uint16_t tci;
        struct {
            uint8_t pcp:3;
            bool dei:1;
            uint16_t vid_unordered:12;
        };
    };
    boost::endian::big_uint16_t type;
    size_t header_length() const
    { return sizeof(*this); }
};
static_assert(sizeof(dot1q_hdr) == 18, "");

struct ipv4_hdr {
    uint8_t ihl:4; // TODO: learn ipv4 protocol
    uint8_t version:4;
    uint8_t dscp:6;
    uint8_t ecn:2;
    boost::endian::big_uint16_t total_len;
    boost::endian::big_uint16_t identification;
    struct {
        uint16_t flags:3;
        uint16_t fragment_offset_unordered:13;
    };

    boost::endian::big_uint8_t ttl;
    boost::endian::big_uint8_t protocol;
    boost::endian::big_uint16_t checksum;
    boost::endian::big_uint32_t src;
    boost::endian::big_uint32_t dst;

    size_t header_length() const
    { return ihl * 4; }
};
static_assert(sizeof(ipv4_hdr) == 20, "");

struct ipv6_hdr {
    union{
        uint32_t startheader;
        // !!! TODO
        /*struct{
            uint8_t version:4;
            uint8_t Traffic_class:8;
            uint32_t FlowLabel:20;
        };*/
    };
    boost::endian::big_uint16_t payload_lenght;
    uint8_t protocol;
    uint8_t HopLimit;
    boost::endian::big_uint64_t src1;
    boost::endian::big_uint64_t src2;
    boost::endian::big_uint64_t dst1;
    boost::endian::big_uint64_t dst2;
    size_t header_length() const
    { return sizeof(ipv6_hdr); }
};
static_assert(sizeof(ipv6_hdr) == 40, "");

struct tcp_hdr {
    boost::endian::big_uint16_t src;
    boost::endian::big_uint16_t dst;
    boost::endian::big_uint32_t seq_no;
    boost::endian::big_uint32_t ack_no;
    uint8_t data_offset:4;
    uint8_t :3; // reserved
    bool NS:1;
    bool CWR:1;
    bool ECE:1;
    bool URG:1;
    bool ACK:1;
    bool PSH:1;
    bool RST:1;
    bool SYN:1;
    bool FIN:1;
    boost::endian::big_uint16_t window_size;
    boost::endian::big_uint16_t checksum;
    boost::endian::big_uint16_t urgent_pointer;
};
static_assert(sizeof(tcp_hdr) == 20, "");

struct udp_hdr {
    boost::endian::big_uint16_t src;
    boost::endian::big_uint16_t dst;
    boost::endian::big_uint16_t length;
    boost::endian::big_uint16_t checksum;
};
static_assert(sizeof(udp_hdr) == 8, "");

struct arp_hdr {
    boost::endian::big_uint16_t htype;
    boost::endian::big_uint16_t ptype;
    boost::endian::big_uint8_t hlen;
    boost::endian::big_uint8_t plen;
    boost::endian::big_uint16_t oper;
    boost::endian::big_uint48_t sha;
    boost::endian::big_uint32_t spa;
    boost::endian::big_uint48_t tha;
    boost::endian::big_uint32_t tpa;
};
static_assert(sizeof(arp_hdr) == 28, "");

struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    boost::endian::big_uint16_t checksum;
};
static_assert(sizeof(icmp_hdr) == 4, "");

// TODO: } // namespace hdr
} // namespace runos
