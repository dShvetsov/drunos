#include "STP.hh"

#include <sstream>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include "Common.hh"
#include "CommandLine.hh"
#include "Switch.hh"

using namespace cli;
using namespace runos;

class StpCli: public Application {
SIMPLE_APPLICATION(StpCli, "stp-cli")
public:
    void init(Loader* loader, const Config& config) override 
    {
        auto app = STP::get(loader);
        auto sw_mgr = SwitchManager::get(loader);
        auto cli = CommandLine::get(loader);

        options::options_description desc;
        desc.add_options()
            ("show,s", "Show stp topology")
            ("disabled,d", "Show disabled ports");

        auto cmd = [=](const options::variables_map& vm, Outside& out) {
            if (not vm["show"].empty()) {
                out.print("Stp tree");
                for (auto sw: sw_mgr->switches()){
                    auto stp_ports = app->getSTP(sw->id());
                    out.echo("switch 0x{:x}:", sw->id());
                    for (auto port: stp_ports) {
                        out.echo(" {}", port);
                    }
                    out.print("");
                }
            } else if (not(vm["disabled"].empty())) {
                out.print("Disabled ports");
                for (auto sw: sw_mgr->switches()){
                    auto stp_ports = app->getDisabledPorts(sw->id());
                    out.echo("switch 0x{:x}:", sw->id());
                    for (auto port: stp_ports) {
                        out.echo(" {}", port);
                    }
                    out.print("");
                }
            } else {
                out.warning("No commands, type help stp for details");
            }
        };
        cli->registerCommand("stp", std::move(desc), std::move(cmd), "Command fot spanning tree application");
    }
};

REGISTER_APPLICATION(StpCli, {"stp", "switch-manager", "command-line-interface", ""})
