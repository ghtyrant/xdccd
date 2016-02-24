#include <iostream>
#include <restbed>
#include <functional>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "api.h"

using namespace std::placeholders;

xdccd::API::API()
    : manager(10)
{
}

xdccd::BotManager &xdccd::API::get_bot_manager()
{
    return manager;
}

void xdccd::API::test_handler(std::shared_ptr<restbed::Session> session)
{
    std::cout << "test_handler!" << std::endl;

    boost::property_tree::ptree root;
    boost::property_tree::ptree bot_list;
    
    std::vector<xdccd::DCCBotPtr> bots = manager.get_bots();
    for(auto it = bots.begin(); it != bots.end(); ++it)
    {
        boost::property_tree::ptree child;
        child.put("nick", (*it)->get_nickname());
        child.put("server", (*it)->get_host() + ":" + (*it)->get_port());
        bot_list.push_back(std::make_pair("", child));
    }

    root.add_child("bots", bot_list);

    std::ostringstream oss;
    boost::property_tree::write_json(oss, root);

    std::cout << "JSON: '" << oss.str() << "'" << std::endl;


    session->close(restbed::OK, oss.str());
}

void xdccd::API::run()
{
    auto resource = std::make_shared<restbed::Resource>();
    resource->set_path("/test");
    resource->set_method_handler("GET", std::bind(&API::test_handler, this, std::placeholders::_1));

    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(1984);
    settings->set_default_header("Connection", "close");

    service.publish(resource);
    service.start(settings);
}
