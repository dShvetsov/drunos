#pragma once

#include <ostream>
#include "types/bits.hh"

namespace runos {
namespace types {

std::ostream& print_eth_type(std::ostream& out, const bits<>& bits);
std::ostream& print_ip_proto(std::ostream& out, const bits<>& bits);
std::ostream& print_arp_op(std::ostream& out, const bits<>& bits);

} // namespace types
} // namespace runos
