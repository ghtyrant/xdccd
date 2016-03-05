#pragma once

#include <restbed>

#include "botmanager.h"
#include "searchcache.h"

namespace xdccd
{

class API
{

    public:
        static bool quit;

        API();
        void run();
        void stop();

        BotManager &get_bot_manager();

        // Static file handler
        void static_file_handler(std::shared_ptr<restbed::Session> session);

        // Resource handlers
        void status_handler(std::shared_ptr<restbed::Session> session);
        void connect_handler(std::shared_ptr<restbed::Session> session);
        void disconnect_handler(std::shared_ptr<restbed::Session> session);
        void remove_file_from_list_handler(std::shared_ptr<restbed::Session> session);
        void request_file_handler(std::shared_ptr<restbed::Session> session);
        void search_handler(std::shared_ptr<restbed::Session> session);
        void shutdown_handler(std::shared_ptr<restbed::Session> session);

    private:
        BotManager manager;
        SearchCache search;
        restbed::Service service;
};

}
