#include "Application.hh"
#include "Common.hh"
#include "Retic.hh"
#include "retic/policies.hh"


using namespace runos;
using namespace retic;

class TestApps: public Application {
SIMPLE_APPLICATION(TestApps, "test-apps")
public:
    void init(Loader* loader, const Config& config) override
    {
        auto retic = Retic::get(loader);
        const static auto in_port = oxm::in_port();
        const static auto switch_id = oxm::switch_id();
        retic->registerPolicy("twoports", fwd(1) + fwd(2));
        retic->registerPolicy("twoinports", (filter(in_port == 2) >> fwd(1)) +
                                          (filter(in_port == 1) >> fwd(2)));
        retic->registerPolicy("dropall", stop());
        retic->registerPolicy("two_switch",
            filter(switch_id == 1) >> (fwd(1) + fwd(2)) | filter(switch_id == 2) >> (fwd(1) + fwd(2))
        );
    }
};

REGISTER_APPLICATION(TestApps, {"retic", ""})

