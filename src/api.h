#pragma once

#include <restbed>

namespace xdccd
{

class API
{
    public:
        API();
        void run();

        // Resource handlers
        void test_handler(std::shared_ptr<restbed::Session> session);

    private:
        restbed::Service service;

};

}
