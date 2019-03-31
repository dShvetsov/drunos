#include "Retic.hh"

#include <sstream>
#include <boost/lexical_cast.hpp>

#include "Common.hh"
#include "CommandLine.hh"

#include "retic/policies.hh"

using namespace cli;
using namespace runos;

class ReticCli: public Application {
SIMPLE_APPLICATION(ReticCli, "retic-cli")
public:
    void init(Loader* loader, const Config& config) override 
    {
        auto app = Retic::get(loader);
        auto cli = CommandLine::get(loader);
        options::options_description desc;
        desc.add_options()
            ("main,m", "Show main function")
            ("verbose,v", "Show main implementation")
            ("policies,p", "Show registered policies")
            ("clear,c", "Clear rules")
            ("reinstall,r", "Reinstall rules of policy")
            ("set_main,s", options::value<std::string>(), "set main function");

        auto cmd = [app](const options::variables_map& vm, Outside& out) {
            if (not vm["main"].empty()) {
               out.print("Main function: {}", app->getMainName());
                if (not vm["verbose"].empty()) {
                    std::stringstream ss;
                    out.print(
                        "Implementation: {}",
                        boost::lexical_cast<std::string>(app->getMainPolicy())
                    );
                }
            }
            if (not vm["policies"].empty()) {
                for (auto& name: app->getPoliciesName()) {
                    out.print("{}", name);
                }
            }
            if (not vm["clear"].empty()) {
                app->clearRules();
            }
            if (not vm["reinstall"].empty()) {
                app->reinstallRules();
            }
            if (not vm["set_main"].empty()) {
                std::string new_main = vm["set_main"].as<std::string>();
                app->setMain(new_main);
            }
        };
        cli->registerCommand("retic", std::move(desc), std::move(cmd), "Retic commands");
    }
};

REGISTER_APPLICATION(ReticCli, {"retic", "command-line-interface", ""})
