#pragma once

#include <mutex>

#include "dccfile.h"
#include "ircconnection.h"
#include "threadpool.h"

namespace xdccd
{

class IRCConnection;
class IRCMessage;

void launch_bot_task();

class DCCBot
{
    public:
        DCCBot(ThreadpoolPtr threadpool, const std::string &host, const std::string &port, const std::string &nick, bool use_ssl);
        void read_handler(const std::string &message);
        void run();
        void on_connected();
        void on_ctcp(const xdccd::IRCMessage &msg);
        void on_join(const std::string &channel);
        void on_part(const std::string &channel);
        const std::vector<std::string> get_channels() const;
        const std::string &get_nickname() const;
        const std::string &get_host() const;
        const std::string &get_port() const;

    private:
        std::string nickname;
        xdccd::IRCConnection connection;
        std::vector<DCCFilePtr> files;
        std::mutex files_lock;
        ThreadpoolPtr threadpool;
        std::vector<std::string> channels;
};

typedef std::shared_ptr<DCCBot> DCCBotPtr;

}
