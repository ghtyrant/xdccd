#pragma once

#include <vector>

#include "threadmanager.h"
#include "dccbot.h"

namespace xdccd
{

class BotManager
{
    public:
        BotManager(ThreadManager &thread_man, std::size_t max_bots);
        ~BotManager();
        void launch_bot(const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl, DownloadManager &download_manager);
        void run();
        std::vector<DCCBotPtr> get_bots();
        DCCBotPtr get_bot_by_id(bot_id_t id);
        void stop_bot(DCCBotPtr bot);

    private:
        std::size_t max_bots;
        std::size_t last_bot_id;
        ThreadManager &thread_manager;
        std::vector<DCCBotPtr> bots;
        std::mutex bots_lock;
};

}
