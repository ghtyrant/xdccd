#pragma once

#include <vector>

#include "threadpool.h"
#include "dccbot.h"

namespace xdccd
{

class BotManager
{
    public:
        BotManager(std::size_t max_bots);
        ~BotManager();
        void launch_bot(const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl);
        void run();
        const std::vector<DCCBotPtr> &get_bots();
        DCCBotPtr get_bot_by_id(bot_id_t id);
        void stop_bot(DCCBotPtr bot);

    private:
        std::size_t max_bots;
        std::size_t last_bot_id;
        Threadpool threadpool;
        std::vector<DCCBotPtr> bots;
};

}
