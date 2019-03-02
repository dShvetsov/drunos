#include "Application.hh"
#include "Loader.hh"
#include "Decision.hh"
#include "Maple.hh"
#include "api/Packet.hh"

class NoMapleRules : public Application {
SIMPLE_APPLICATION(NoMapleRules, "no-maple-rules")
public:
    void init(Loader* loader, const Config& config) override
    {
        auto maple = Maple::get(loader);
        maple->registerHandler("no-maple-rules",
            [=](Packet& pkt, FlowPtr, Decision decision) {
                return decision.hard_timeout(std::chrono::seconds::zero());
            }
        );
    }
};

REGISTER_APPLICATION(NoMapleRules, {"maple", ""})
