# What is dRunos

dRunos is an OpenFlow Controller.

It is fully userspace controller with high functionality, easy to develop your apps, relatively high performance comparing with existing controllers.
It supports OpenFlow 1.3.


# Environment

Instead of installing all the dependencies we recommend to use a pre-built docker container, with all dependecies installed.


```
./drunosdev
```

Note: We don't provide pre-built drunos binary because the main purpose of drunos is high-level domain specific language for defining network application, and we don't support plugins now.


# Building

```
# Initialize third-party libraries
$ third_party/bootstrap.sh

# Create out of source build directory
$ mkdir -p build; cd build
# Configure
$ cmake -DCMAKE_BUILD_TYPE=Release ..

# Build third-party libraries
$ make prefix -j2
# Build dRunos
$ make -j2
```

# Running unit test

To run unit test

```
cd build && make test
```

# Running function test

To run functional test with mininet and different application

```
cd functest && pytest simplePingTest.py
```

# Running

Setup environment variables (run once per shell session):
```
# Run it INSIDE build directory
$ source ../debug_run_env.sh
```

Run the controller:
```
$ cd .. # Go out of build dir
$ build/runos
```

You can use this arguments for MiniNet:
```
$ sudo mn --topo $YOUR_TOPO --switch ovsk,protocols=OpenFlow13 \
            --controller remote,ip=$CONTROLLER_IP,port=6653
```

To run web UI, open the following link in your browser:
```
http://$CONTROLLER_IP:8000/topology.html
```

Be sure your MiniNet installation supports OpenFlow 1.3.
See https://wiki.opendaylight.org/view/OpenDaylight_OpenFlow_Plugin::Test_Environment#Setup_CPqD_Openflow_1.3_Soft_Switch for more instructions.

# Writing your first RuNOS app

Note: look at full documentation in Russian: http://arccn.github.io/runos/doc/ru/index.html

## Step 1: Override Application class

Create ``MyApp.cc`` and ``MyApp.hh`` files inside `src` directory and
reference them in `CMakeLists.txt`.

Fill them with the following content:

    /* MyApp.hh */
    #pragma once

    #include "Application.hh"
    #include "Loader.hh"

    class MyApp : public Application {
    SIMPLE_APPLICATION(MyApp, "myapp")
    public:
        void init(Loader* loader, const Config& config) override;
    };

    /* MyApp.cc */
    #include "MyApp.hh"

    REGISTER_APPLICATION(MyApp, {""})

    void MyApp::init(Loader *loader, const Config& config)
    {
        LOG(INFO) << "Hello, world!";
    }

What we do here? We declare our application by subclassing `Application`
interface and named it `myapp`. Now we should reference `myapp` in configuration
file (defaults to `network-setting.json`) at `services` section to force run it.

Start RuNOS and see "Hello, world!" in the log.

## Step 2: Subscribing to other application events

Now our application was started but do nothing. To make it
useful you need to communicate with other applications.

Look at this line of `MyApp.cc`:

    REGISTER_APPLICATION(MyApp, {""})


In the second argument here you tell RuNOS what apps you will use as dependencies.
Last element should be empty string indicating end-of-list.

You can interact with other applications with *method calls* and *Qt signals*. Let's
subscribe to switchUp event of `controller` application. Add required dependencies:

    REGISTER_APPLICATION(MyApp, {"controller", ""})


Then include `Controller.hh` in `MyApp.cc` and get controller instance:


    void MyApp::init(Loader *loader, const Config& config)
    {
        Controller* ctrl = Controller::get(loader);

        ...
    }



Now we need a slot to process `switchUp` events. Declare it in `MyApp.hh`:
    #include "SwitchConnection.hh"
    ...

    public slots:
        void onSwitchUp(SwitchConnectionPtr ofconn, of13::FeaturesReply fr);

And connect signal to it in `MyApp::init`:

        QObject::connect(ctrl, &Controller::switchUp,
                         this, &MyApp::onSwitchUp);

Finally, write `MyApp::onSwitchUp` implementation:

    void MyApp::onSwitchUp(SwitchConnectionPtr ofconn, of13::FeaturesReply fr)
    {
        LOG(INFO) << "Look! This is a switch " << fr.datapath_id();
    }

Great! Now you will see this greeting when switch connects to RuNOS.

## Step 3: Working with data flows

You learned how subscribe to other application events, but how to manage flows?
Imagine you wan't to do MAC filtering, ie drop all packets from hosts not listed
in the configuration file.

To do so you need to create packet-in handler.

So, register PacketMissHandlerFactory in `init`:

    #include "api/PacketMissHandler.hh"
    #include "api/Packet.hh"
    #include "types/ethaddr.hh"
    #include "api/TraceablePacket.hh"
    #include "oxm/openflow_basic.hh"
    #include "Maple.hh"

    REGISTER_APPLICATION(MyApp, {"controller", "maple", ""})

    void MyApp::init(Loader *loader, const Config& config)
    {
        ...

        Maple* maple = Maple::get(loader);
        maple->registerHandler("mac-filtering",
            [=](Packet& pkt, FlowPtr, Decision decision){
		if (pkt.test(oxm::eth_src() == "00:11:22:33:44:55"))
		     return decision.drop().return_();
		else
                     return decision;
        });

        ...
    }

What it means? First, all handlers arranged into pipeline, where every handler
can stop processing, look at some packet fields and add actions. We need also
insert our handler in pipeline. In `network-settings.json` `maple` has his setting,
and pariculary `pipeline`, which contains name of handlers in pipeline.


So, we name our handler as "mac-filtering" and need to place it before "forwarding".

Now compile RuNOS and test that all packets from ``00:11:22:33:44:55`` had been dropped.

# REST Applications

## List of available REST services

The format of the RunOS REST requests:

    <HTTP-method> /api/<app_name>/<list_of_params>

* `<HTTP-method>` is GET, POST, DELETE of PUT
* `<app_name>` is calling name of the application
* `<list_of_params>` is list of the parameters separated by a slash

In POST and PUT request you can pass parameters in the body of the request using JSON format.

Current version of RunOS has 6 REST services:
* switch-manager
* topology
* host-manager
* flow
* static-flow-pusher
* stats

### 'Switch Manager'

    GET /api/switch-manager/switches/all 	(RunOS version)
    GET /wm/core/controller/switches/json 	(Floodlight version)
Return the list of connected switches

### 'Topology'

    GET /api/topology/links			    (RunOS version)
    GET /wm/topology/links/json 		(Floodlight version)
Return the list of all the inter-switch links

    GET /wm/topology/external-links/json 	(Floodlight)
Return external links

### 'Host Manager'

    GET /api/host-manager/hosts		(RunOS)
    GET /wm/device/			        (Floodlight)
List of all end hosts tracked by the controller

### 'Flow Manager'

    GET /api/flow/<switch_id>
    DELETE /api/flow/<switch_id>/<flow_id>
List flow entries in the switch and remove some flow entry

### 'Static Flow Pusher'

    POST /api/static-flow-pusher/newflow/<switch_id>
    body of request: JSON description of new flow
Create new flow entry in the some switch

### 'Stats'

    GET /api/stats/port_info/<switch_id>/all
    GET /api/stats/port_info/<switch_id>/<port_id>
Get switch port statistics

### Other

    GET /apps
List of available applications with REST API.

Also you can get events in the applications that subscribed to event model.
In this case you should specify list of required applications and your last registered number of events.

    GET /timeout/<app_list>/<last_event>
* `<app_list>` is `app_1&app_2&...&app_n`
* `<last_event>` is `unsigned integer` value


## Examples

For testing your and other REST application's requests and replies you can use `cURL`.
You can install this component by `sudo apt-get install curl` command in Ubuntu.

In mininet topology `--topo tree,2`, for example:

Request: `$ curl $CONTROLLER_IP:$LISTEN_PORT/api/switch-manager/switches/all`
Reply: `[{"DPID": "0000000000000001", "ID": "0000000000000001"}, {"DPID": "0000000000000002", "ID": "0000000000000002"}, {"DPID": "0000000000000003", "ID": "0000000000000003"}]`

## Adding REST for your RunOS app

To make REST for your application `class MyApp`:

* add REST for your application:

        #include "Rest.hh"
        #include "AppObject.hh"
        #include <string>
        class MyApp : public Application, RestHandler {
            bool eventable() override { return false; }
            std::string displayedName() override { return "My beautiful application"; } // or skip this
            std::string page() override { return "my_page.html"; } // if your application has webpage

            json11::Json handleGET(std::vector<std::string> params, std::string body) override;
            json11::Json handlePOST(std::vector<std::string> params, std::string body) override;
            // also handlePUT and handleDELETE
        }

* in `init()` function in `class MyApp` register Rest handler:

        RestListener::get(loader)->registerRestHandler(this);

* then, you should set the handled pathes and methods in MyApp::init:

    // handle GET request with path /api/my-app/switches/<number>
    acceptPath(Method::GET, "switches/[0-9]+");

    // handle POST request with path /api/my-app/mac/<string>
    acceptPath(Method::POST, "[A-Fa-f0-9:-]");

* method `MyApp::handleGET` proccesses input GET-HTTP requests.
  This method gets the list of parameters and returns reply in JSON format.

* each REST application may have own webpage, i.e. WebUI for application
  To connect REST application to webpage you must specify this page in `page()` method:

        std::string page() override { return "my_page.html"; }

  File `"my_page.html"` must be located in `web/html` directory.
  If your application has not WebUI, write `"none"` instead webpage.

* if your application supports event model, you can enable it by setting `true` in `eventable()` method:

        bool eventable() override { return true; }

In current version events can signal about appearance or disappearance some objects:

    # some_object is object inherited class AppObject
    addEvent(Event::Add, some_object);
    # or
    addEvent(Event::Delete, some_object);

## Static Flow Pusher

You can use static flow pusher to set rules proactively.
At first, you can write required rules in `network-settings.json` file.
For example:

    "static-flow-pusher": {
      "flows": [
        {
        "dpid": "all",
        "flows": [
            {
              "priority": 0,
              "in_port": 15,
              "ip_src": "3.3.3.3",
              "out_port" : 22
            }
         ]
         }
      ]
    }

Also, you can add other match fields: `in_port`, `eth_src`, `eth_dst`, `ip_src`, `ip_dst`, `out_port`. To set idle and hard timeout use `idle` and `hard` strings. To set `OFPP_TO_CONTROLLER` action, write in `out_port` string value `to-controller`.

Secondly, to set flows proactively, you should add to your application StaticFlowPusher application, create FlowDesc object, fill it with your match field and call `&StaticFlowPusher::sendToSwitch` method.

And thirdly, use REST POST requests of StaticFlowPusher to set new flow from REST API.
