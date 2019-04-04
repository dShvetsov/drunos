#pragma once

#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"
#include "retic/backend.hh"

using namespace runos;
using namespace retic;

template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };

struct MockBackend: public Backend {
    MOCK_METHOD4(install,
        void(
            oxm::field_set,
            std::vector<oxm::field_set>,
            uint16_t,
            FlowSettings
        )
    );
    MOCK_METHOD2(installBarrier, void(oxm::field_set, uint16_t));
    MOCK_METHOD4(packetOuts,
        void(
            uint8_t* data,
            size_t data_len,
            std::vector<oxm::field_set>,
            uint64_t
        )
    );
};
