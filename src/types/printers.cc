#include "printers.hh"

#include "packet_headers.hh"

namespace runos {
namespace types {

std::ostream& print_eth_type(std::ostream& out, const bits<>& bits) {
    uint16_t eth_type = bit_cast<uint16_t>(bits);
    switch(eth_type) {
        case 0x8100: return out << "vlan_tag";
        case 0x0800: return out << "ipv4";
        case 0x0806: return out << "arp";
        case 0x86dd: return out << "ipv6";
        case 0x88cc: return out << "lldp";
        default: return out << "0x" << std::hex << eth_type;
    }
}

std::ostream& print_ip_proto(std::ostream& out, const bits<>& bits) {
    uint8_t ip_proto = bit_cast<uint8_t>(bits);
    switch(ip_proto) {
        case 0x06: return out << "tcp";
        case 0x11: return out << "udp";
        case 0x01: return out << "icmp";
        default: return out << "0x" << std::hex << ip_proto;
    }
}

std::ostream& print_arp_op(std::ostream& out, const bits<>& bits) {
    uint16_t oper = bit_cast<uint16_t>(bits);
    switch(oper) {
        case 1: return out << "request";
        case 2: return out << "reply";
        default: return out << "unknown(" << oper << ")";
    }
}

} // namespace types
} // namespace runos
