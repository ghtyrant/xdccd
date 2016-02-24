#include <iostream>
#include <restbed>
#include <functional>

#include "api.h"

using namespace std::placeholders;

xdccd::API::API()
{
}

void xdccd::API::test_handler(std::shared_ptr<restbed::Session> session)
{
    std::cout << "test_handler!" << std::endl;
    session->close(restbed::OK, "Hello!");
}

void xdccd::API::run()
{
    auto resource = std::make_shared<restbed::Resource>();
    resource->set_path("/test");
    resource->set_method_handler("GET", std::bind(&API::test_handler, this, _1));

    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(1984);
    settings->set_default_header("Connection", "close");

    service.publish(resource);
    service.start(settings);
}
