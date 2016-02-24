#pragma once

#include <restbed>

#include "botmanager.h"

namespace xdccd
{

class API
{
    public:
        API();
        void run();

        BotManager &get_bot_manager();

        // Resource handlers
        void test_handler(std::shared_ptr<restbed::Session> session);

    private:
        BotManager manager;
        restbed::Service service;

};

}
