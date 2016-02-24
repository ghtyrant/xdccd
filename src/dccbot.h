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
        void handle_ctcp(const xdccd::IRCMessage &msg);
        void run();
        void on_connected();
        void on_joined(const std::string &channel);

    private:
        xdccd::IRCConnection connection;
        std::vector<DCCFilePtr> files;
        std::mutex files_lock;
        ThreadpoolPtr threadpool;
};

typedef std::shared_ptr<DCCBot> DCCBotPtr;

}
