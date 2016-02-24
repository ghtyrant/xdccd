#include "botmanager.h"

xdccd::BotManager::BotManager(std::size_t max_bots)
    : max_bots(max_bots), threadpool(std::make_shared<Threadpool>(max_bots))
{}

void xdccd::BotManager::launch_bot(const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl)
{
    DCCBotPtr bot = std::make_shared<DCCBot>(threadpool, host, port, nick, use_ssl);
    bots.push_back(bot);

    threadpool->run_task([bot]() { bot->run(); });
}

void xdccd::BotManager::run()
{
    api.run();
}


