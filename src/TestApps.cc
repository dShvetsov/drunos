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
        retic->registerPolicy("twoports", fwd(1) + fwd(2));
        retic->registerPolicy("twoinports", (filter(in_port == 2) >> fwd(1)) +
                                          (filter(in_port == 1) >> fwd(2)));
        retic->registerPolicy("dropall", stop());
    }
};

REGISTER_APPLICATION(TestApps, {"retic", ""})

