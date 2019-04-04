#pragma once

#include <ostream>

#include "fdd.hh"

namespace runos {
namespace retic {

void dumpAsDot(const fdd::diagram& d, std::ostream& out);

}
}
