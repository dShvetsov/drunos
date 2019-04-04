/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <fstream>
#include <sstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "json11.hpp"

#include "Common.hh"
#include "Config.hh"
#include "Flow.hh"
#include "Loader.hh"
#include "SwitchConnection.hh"

using namespace json11;

Config loadConfig(const std::string& fileName,
                  const std::string& profile)
{
    std::stringstream buffer;
    std::string parseMessage;

    buffer << std::ifstream(fileName).rdbuf();
    Config config =
        Json::parse(buffer.str(), parseMessage)[profile].object_items();

    if (!parseMessage.empty()) {
        LOG(FATAL) << "Can't parse config file : " << parseMessage;
    }

    return config;
}


int main(int argc, char* argv[]) {
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<of13::PortStatus>();
    qRegisterMetaType<of13::FeaturesReply>();
    qRegisterMetaType<of13::FlowRemoved>();
    qRegisterMetaType< std::shared_ptr<of13::Error> >();
    qRegisterMetaType<of13::Port>();
    qRegisterMetaType<of13::Match>();
    qRegisterMetaType<runos::SwitchConnectionPtr>("SwitchConnectionPtr");
    qRegisterMetaType<runos::Flow::State>("State");

    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    QCoreApplication app(argc, argv);

    namespace po = boost::program_options;

    std::string configFile = "network-settings.json";
    std::string profile = "default";

    po::options_description desc("RUNOS options");
    desc.add_options()
        ("help,h", "Show help and exit")
        ("config,c", po::value<std::string>(&configFile)->default_value("network-settings.json"), "Set a settings file")
        ("profile,f", po::value<std::string>(&profile)->default_value("default"), "Set a profile of setting file");
    po::variables_map vm;
    po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed, vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc;
        return 0;
    }
    Config config = loadConfig(configFile, profile);

    Loader loader(config);
    loader.startAll();

    return app.exec();
}
