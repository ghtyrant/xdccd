#include "botmanager.h"

xdccd::BotManager::BotManager(std::size_t max_bots)
    : max_bots(max_bots), last_bot_id(0), threadpool(std::make_shared<Threadpool>(max_bots))
{}

void xdccd::BotManager::launch_bot(const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl)
{
    DCCBotPtr bot = std::make_shared<DCCBot>(last_bot_id++, threadpool, host, port, nick, channels, use_ssl);
    bots.push_back(bot);

    threadpool->run_task([bot]() { bot->run(); });
}

const std::vector<xdccd::DCCBotPtr> &xdccd::BotManager::get_bots()
{
    return bots;
}

void xdccd::BotManager::run()
{
}

xdccd::DCCBotPtr xdccd::BotManager::get_bot_by_id(bot_id_t id)
{
    for (auto bot : bots)
        if (bot->get_id() == id)
            return bot;

    return nullptr;
}

void xdccd::BotManager::stop_bot(xdccd::DCCBotPtr bot)
{
    bot->stop();
    bots.erase(std::remove(bots.begin(), bots.end(), bot));
}
