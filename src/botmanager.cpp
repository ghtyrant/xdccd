#include <boost/log/trivial.hpp>

#include "botmanager.h"

xdccd::BotManager::BotManager(ThreadManager &thread_man, std::size_t max_bots)
    : max_bots(max_bots), last_bot_id(0), thread_manager(thread_man)
{}

xdccd::BotManager::~BotManager()
{
    std::lock_guard<std::mutex> lock(bots_lock);
    for (auto &bot : bots)
        bot->stop();
}

void xdccd::BotManager::launch_bot(const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl, DownloadManager &download_manager)
{
    DCCBotPtr bot = std::make_shared<DCCBot>(last_bot_id++, host, port, nick, channels, use_ssl, download_manager);

    BOOST_LOG_TRIVIAL(info) << "Launching bot " << bot;

    thread_manager.run([bot]() { bot->run(); });
    std::lock_guard<std::mutex> lock(bots_lock);
    bots.push_back(bot);
}

std::vector<xdccd::DCCBotPtr> xdccd::BotManager::get_bots()
{
    std::lock_guard<std::mutex> lock(bots_lock);
    return bots;
}

void xdccd::BotManager::run()
{
}

xdccd::DCCBotPtr xdccd::BotManager::get_bot_by_id(bot_id_t id)
{
    std::lock_guard<std::mutex> lock(bots_lock);

    for (auto &bot : bots)
        if (bot->get_id() == id)
            return bot;

    return nullptr;
}

void xdccd::BotManager::stop_bot(xdccd::DCCBotPtr bot)
{
    bot->stop();

    std::lock_guard<std::mutex> lock(bots_lock);
    bots.erase(std::remove(bots.begin(), bots.end(), bot));
}
