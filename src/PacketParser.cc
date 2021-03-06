#include "PacketParser.hh"

#include <cstring>
#include <algorithm>

#include <boost/endian/arithmetic.hpp>
#include <boost/exception/error_info.hpp>

#include <fluid/of13msg.hh>

#include "types/exception.hh"
#include "types/packet_headers.hh"
#include "Common.hh"

using namespace boost::endian;

namespace runos {

typedef boost::error_info< struct tag_oxm_ns, unsigned >
    errinfo_oxm_ns;
typedef boost::error_info< struct tag_oxm_field, unsigned >
    errinfo_oxm_field;

using ofb = of::oxm::basic_match_fields;
using non_of = of::oxm::non_openflow_fields;


// TODO: make it more safe
// add size-checking of passed oxm type against field size
void PacketParser::bind(ofb_binding_list new_bindings)
{
    for (const auto& binding : new_bindings) {
        auto id = static_cast<size_t>(binding.first);
        if (ofb_bindings.at(id)) {
            RUNOS_THROW(
                    invalid_argument() <<
                    errinfo_msg("Trying to bind already binded field") <<
                    errinfo_oxm_ns(uint16_t(of::oxm::ns::OPENFLOW_BASIC)) <<
                    errinfo_oxm_field(id));
        }
        ofb_bindings.at(id) = binding.second;
    }
}

void PacketParser::rebind(ofb_binding_list new_bindings)
{
    for (const auto& binding : new_bindings) {
        auto id = static_cast<size_t>(binding.first);
        if (!ofb_bindings.at(id)) {
            RUNOS_THROW(
                    invalid_argument() <<
                    errinfo_msg("Trying to rebind unbinded field") <<
                    errinfo_oxm_ns(uint16_t(of::oxm::ns::OPENFLOW_BASIC)) <<
                    errinfo_oxm_field(id));
        }
        ofb_bindings.at(id) = binding.second;
    }
}

void PacketParser::bind(nonof_binding_list new_bindings)
{
    for (const auto& binding : new_bindings) {
        auto id = static_cast<size_t>(binding.first);
        if (nonof_bindings.at(id)) {
            RUNOS_THROW(
                    invalid_argument() <<
                    errinfo_msg("Trying to bind already binded field") <<
                    errinfo_oxm_ns(uint16_t(of::oxm::ns::NON_OPENFLOW)) <<
                    errinfo_oxm_field(id));
        }
        nonof_bindings.at(id) = binding.second;
    }
}

void PacketParser::rebind(nonof_binding_list new_bindings)
{
    for (const auto& binding : new_bindings) {
        auto id = static_cast<size_t>(binding.first);
        if (!nonof_bindings.at(id)) {
            RUNOS_THROW(
                    invalid_argument() <<
                    errinfo_msg("Trying to rebind unbinded field") <<
                    errinfo_oxm_ns(uint16_t(of::oxm::ns::NON_OPENFLOW)) <<
                    errinfo_oxm_field(id));
        }
        nonof_bindings.at(id) = binding.second;
    }
}


void PacketParser::parse_l2(uint8_t* data, size_t data_len)
{
    uint16_t type = 0; // For adequate compile
    size_t len = 0; // For adequate compile
    if (sizeof(ethernet_hdr) <= data_len) {
        eth = reinterpret_cast<ethernet_hdr*>(data);
        if (eth->type == 0x8100){
            if (sizeof(dot1q_hdr) <= data_len ){
                dot1q = reinterpret_cast<dot1q_hdr*>(data);
                bind({
                    { ofb::ETH_TYPE, &dot1q->type },
                    { ofb::ETH_SRC, &dot1q->src },
                    { ofb::ETH_DST, &dot1q->dst },
                    { ofb::VLAN_VID, &dot1q->tci } //fix this
                });
                type = dot1q->type;
                len = dot1q->header_length();
            }
        } else {
            bind({
                { ofb::ETH_TYPE, &eth->type },
                { ofb::ETH_SRC, &eth->src },
                { ofb::ETH_DST, &eth->dst }
            });
            type = eth->type;
            len = eth->header_length();
        }
    parse_l3(type,
             static_cast<uint8_t*>(data) + len,
             data_len - len);
    }
}

void PacketParser::parse_l3(uint16_t eth_type, uint8_t* data, size_t data_len)
{
    switch (eth_type) {
    case 0x0800: // ipv4
        if (sizeof(ipv4_hdr) <= data_len) {
            ipv4 = reinterpret_cast<ipv4_hdr*>(data);
            bind({
                { ofb::IP_PROTO, &ipv4->protocol },
                { ofb::IPV4_SRC, &ipv4->src },
                { ofb::IPV4_DST, &ipv4->dst }
            });

            if (data_len > ipv4->header_length()) {
                parse_l4(ipv4->protocol,
                         data + ipv4->header_length(),
                         data_len - ipv4->header_length());
            }
        }
        break;
    case 0x0806: // arp
        if (sizeof(arp_hdr) <= data_len) {
            arp = reinterpret_cast<arp_hdr*>(data);
            if (arp->htype != 1 ||
                arp->ptype != 0x0800 ||
                arp->hlen != 6 ||
                arp->plen != 4)
                break;

            bind({
                { ofb::ARP_OP, &arp->oper },
                { ofb::ARP_SHA, &arp->sha },
                { ofb::ARP_THA, &arp->tha },
                { ofb::ARP_SPA, &arp->spa },
                { ofb::ARP_TPA, &arp->tpa }
            });
        }
        break;
    case 0x86dd: // ipv6
        if (sizeof(ipv6_hdr) <= data_len){
            ipv6 = reinterpret_cast<ipv6_hdr*>(data);
            bind({
                { ofb::IPV6_SRC, &ipv6->src1 },
                { ofb::IPV6_DST, &ipv6->dst1 },
                { ofb::IP_PROTO, &ipv6->protocol }
            });
        }
        if (data_len > ipv6->header_length()){
            parse_l4(ipv6->protocol,
                     data + ipv6->header_length(),
                     data_len - ipv6->header_length());
        }
        break;
    }
}

void PacketParser::parse_l4(uint8_t protocol, uint8_t* data, size_t data_len)
{
    switch (protocol) {
    case 0x06: // tcp
        if (sizeof(tcp_hdr) <= data_len) {
            tcp = reinterpret_cast<tcp_hdr*>(data);
            bind({
                { ofb::TCP_SRC, &tcp->src },
                { ofb::TCP_DST, &tcp->dst }
            });
        }
        break;
    case 0x11: // udp
        if (sizeof(udp_hdr) <= data_len) {
            udp = reinterpret_cast<udp_hdr*>(data);
            bind({
                { ofb::UDP_SRC, &udp->src },
                { ofb::UDP_DST, &udp->dst }
            });
        }
        break;
    case 0x01: // icmp
        if (sizeof(icmp_hdr) <= data_len) {
            icmp = reinterpret_cast<icmp_hdr*>(data);
            bind({
                { ofb::ICMPV4_TYPE, &icmp->type },
                { ofb::ICMPV4_CODE, &icmp->code }
            });
        }
        break;
    }
}

PacketParser::PacketParser(fluid_msg::of13::PacketIn& pi, uint64_t dpid, uint32_t out)
    : data(static_cast<uint8_t*>(pi.data()))
    , data_len(pi.data_len())
    , in_port(pi.match().in_port()->value())
    , out_port(out)
    , switch_id(dpid)
{
    ofb_bindings.fill(nullptr);
    nonof_bindings.fill(nullptr);
    bind({
        { ofb::IN_PORT, &in_port }
    });
    bind({
        { non_of::SWITCH_ID, &switch_id }
    });
    bind({
        { non_of::OUT_PORT, &out_port }
    });

    if (data) {
        parse_l2(data, data_len);
    }
}

uint8_t* PacketParser::access(oxm::type t) const
{
    uint8_t* ret(nullptr);
    switch (t.ns()){
        case unsigned(of::oxm::ns::OPENFLOW_BASIC) :
            if (t.id() >= ofb_bindings.size() || !ofb_bindings[t.id()]) {
                RUNOS_THROW(
                        out_of_range() <<
                        errinfo_msg("Unsupported oxm field") <<
                        errinfo_oxm_ns(t.ns()) <<
                        errinfo_oxm_field(t.id()));
            }
            ret =  (uint8_t*) ofb_bindings[t.id()];
        break;
        case unsigned(of::oxm::ns::NON_OPENFLOW) :
             if (t.id() >= nonof_bindings.size() || !nonof_bindings[t.id()]) {
                RUNOS_THROW(
                        out_of_range() <<
                        errinfo_msg("Unsupported oxm field") <<
                        errinfo_oxm_ns(t.ns()) <<
                        errinfo_oxm_field(t.id()));
            }
            ret =  (uint8_t*) nonof_bindings[t.id()];
        break;
        default:
            RUNOS_THROW(
                    out_of_range() <<
                    errinfo_msg("Unsupported oxm namespace") <<
                    errinfo_oxm_ns(t.ns()));
        break;
    }
    if ( !ret ){
        RUNOS_THROW(
                out_of_range() <<
                errinfo_msg("Couldn't find value") <<
                errinfo_oxm_ns(t.ns()) <<
                errinfo_oxm_field(t.id()));
    }
    return ret;

}

oxm::field<> PacketParser::load(oxm::mask<> mask) const
{
    auto value_bits = bits<>(mask.type().nbits(), access(mask.type()));
    return oxm::value<>{ mask.type(), value_bits } & mask;
}

void PacketParser::modify(oxm::field<> patch)
{
    oxm::field<> updated =
        PacketParser::load(oxm::mask<>(patch.type())) >> patch;
    updated.value_bits().to_buffer(access(patch.type()));
}

size_t PacketParser::serialize_to(size_t buffer_size, void* buffer) const
{
    size_t copied = std::min(data_len, buffer_size);
    std::memmove(buffer, data, copied);
    return copied;
}

size_t PacketParser::total_bytes() const
{
    return data_len;
}

// TODO: testme
std::unique_ptr<Packet> PacketParser::clone() const {
    class FluidPacketHolder: public PacketProxy {
    public:
        FluidPacketHolder(std::shared_ptr<of13::PacketIn> pi, std::shared_ptr<Packet> pkt)
        : PacketProxy(*pkt)
        , pi(pi)
        , pkt(pkt)
        { }
    private:
        std::shared_ptr<of13::PacketIn> pi;
        std::shared_ptr<Packet> pkt;
    };

    auto pi = std::make_shared<of13::PacketIn>(0x0, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi->add_oxm_field(new of13::InPort(in_port));
    pi->data(data, data_len);
    auto pkt = std::make_shared<PacketParser>(*pi, switch_id, out_port);
    return std::make_unique<FluidPacketHolder>(pi, pkt);
}

} // namespace runos
