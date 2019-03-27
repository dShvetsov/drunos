#pragma once

#include "fdd.hh"
#include "tracer.hh"

namespace runos {
namespace retic {

std::vector<tracer::Trace> getTraces(const fdd::leaf& l, const Packet& orig_pkt);

} // namespace retic
} // namespace runos
