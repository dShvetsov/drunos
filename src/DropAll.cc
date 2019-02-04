#include "Application.hh"
#include "Loader.hh"
#include "Maple.hh"
#include "Decision.hh"
#include "api/Packet.hh"

class DropAll : public Application {
SIMPLE_APPLICATION(DropAll, "drop-all")
public:
    void init(Loader* loader, const Config& config) override
    {
        auto maple = Maple::get(loader);
        maple->registerHandler("drop-all",
            [=](Packet& pkt, FlowPtr, Decision decision) {
                return decision.drop();
            }
        );
    }
};

REGISTER_APPLICATION(DropAll, {"maple", ""})
