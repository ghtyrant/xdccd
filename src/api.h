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
        void status_handler(std::shared_ptr<restbed::Session> session);
        void connect_handler(std::shared_ptr<restbed::Session> session);
        void disconnect_handler(std::shared_ptr<restbed::Session> session);
        void request_file_handler(std::shared_ptr<restbed::Session> session);
        void search_handler(std::shared_ptr<restbed::Session> session);

    private:
        BotManager manager;
        restbed::Service service;

};

}
