#include "Retic.hh"

#include "Controller.hh"
#include "Common.hh"
#include "oxm/openflow_basic.hh"
#include "PacketParser.hh"

REGISTER_APPLICATION(Retic, {"controller", ""})

using namespace runos;

void Retic::init(Loader* loader, const Config& config)
{
    auto ctrl = Controller::get(loader);
    const auto ofb_in_port = oxm::in_port();

    ctrl->registerHandler<of13::PacketIn>(
            [=](of13::PacketIn& pi, SwitchConnectionPtr conn) {
                LOG(INFO) << "Packet in";
                uint8_t buffer[1500];
                PacketParser pp { pi, conn->dpid() };
                retic::Applier<PacketParser> runtime{pp};
                boost::apply_visitor(runtime, m_policy);
                auto& results = runtime.results();
                for (auto& [p, meta]: results) {
                    const Packet& pkt{p};
                    of13::PacketOut po;
                    po.xid(0x1234);
                    po.buffer_id(OFP_NO_BUFFER);
                    uint32_t out_port = pkt.load(oxm::out_port());
                    LOG(WARNING) << "Output to " << out_port << " port";
                    if (out_port != 0) {
                        po.in_port(pkt.load(oxm::in_port()));
                        po.add_action(new of13::OutputAction(out_port, 0));
                        size_t len = p.total_bytes();
                        p.serialize_to(sizeof(buffer), buffer);
                        po.data(buffer, len);
                        conn->send(po);
                    }
                }
            }
        );

}
