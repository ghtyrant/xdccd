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
        void launch_bot(const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl);
        void run();
        std::vector<DCCBotPtr> get_bots();

    private:
        std::size_t max_bots;
        ThreadpoolPtr threadpool;
        std::vector<DCCBotPtr> bots;
};

}
