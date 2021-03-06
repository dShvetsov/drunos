#pragma once

#include <tuple>

#include "api/Packet.hh"
#include "oxm/field.hh"

namespace runos {

class TraceablePacket {
public:
    // get field without tracing
    virtual oxm::field<> watch(oxm::mask<> mask) const = 0;

    // get field depending by other field
    virtual std::pair< oxm::field<>,
                       oxm::field<> >
            vload(oxm::mask<> by, oxm::mask<> what) const = 0;
};

class TraceableProxy final : public TraceablePacket,
                             public PacketProxy
{
public:
    oxm::field<> watch(oxm::mask<> mask) const override
    {
        return tpkt ? tpkt->watch(mask) : pkt.load(mask);
    }

    template<class Type>
    oxm::value<Type> watch(Type type) const
    {
        auto field = watch(oxm::mask<Type>{type});
        BOOST_ASSERT(field.exact());
        return oxm::value<Type>(field);
    }

    template<class Type>
    oxm::field<Type> watch(oxm::mask<Type> mask) const
    {
        auto generic_mask = static_cast<oxm::mask<>>(mask);
        auto generic_ret = watch(generic_mask);
        return static_cast<oxm::field<Type>>(generic_ret);
    }

    std::pair< oxm::field<>,
               oxm::field<> >
    vload(oxm::mask<> by, oxm::mask<> what) const override
    {
        return tpkt ?
            tpkt->vload(by, what) :
            std::make_pair(pkt.load(by), pkt.load(what));
    }

    template<class Type1, class Type2>
    std::pair<oxm::value<Type1>,
              oxm::value<Type2>>
    vload(Type1 by, Type2 what) const
    {
        auto ret = vload(oxm::mask<Type1>{by}, oxm::mask<Type2>{what});
        // TODO asserts
        return {
            oxm::value<Type1>(ret.first),
            oxm::value<Type2>(ret.second)
        };
    }


    template<class Type1, class Type2>
    std::pair<oxm::field<Type1>,
              oxm::field<Type2> >
    vload(oxm::mask<Type1> by, oxm::mask<Type2> what) const
    {
        auto generic_by = static_cast<oxm::mask<>>(by);
        auto generic_what = static_cast<oxm::mask<>>(what);
        auto ret = vload(generic_by, generic_what);
        return {
            static_cast<oxm::field<Type1>>(ret.first),
            static_cast<oxm::field<Type2>>(ret.second)
        };
    }

    explicit TraceableProxy(Packet& pkt) noexcept
        : PacketProxy(pkt)
        , tpkt(packet_cast<TraceablePacket*>(pkt))
    { }

protected:
    TraceablePacket* const tpkt;
};

// guaranteed downcast
template<class T>
typename std::enable_if<std::is_same<T, TraceablePacket>::value, TraceableProxy>::type
packet_cast(Packet& pkt)
{
    return TraceableProxy(pkt);
}

} // namespace runos
