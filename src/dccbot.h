#pragma once

#include <mutex>

#include "dccfile.h"
#include "ircconnection.h"
#include "threadpool.h"

namespace xdccd
{

class IRCMessage;

typedef std::size_t bot_id_t;

struct DCCAnnounce
{
    DCCAnnounce(bot_id_t bot_id, const std::string &bot_name, const std::string &filename, const std::string &size, const std::string &slot, const std::string &download_count);

    bot_id_t bot_id;
    std::string hash;
    std::string bot_name;
    std::string filename;
    std::string size;
    std::string slot;
    std::string download_count;

    bool compare(const std::string &other) const;
};

typedef std::shared_ptr<DCCAnnounce> DCCAnnouncePtr;

class DCCBot
{
    public:
        DCCBot(bot_id_t id, ThreadpoolPtr threadpool, const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl);
        ~DCCBot();
        void read_handler(const std::string &message);
        void run();
        void stop();
        void on_connected();
        void on_ctcp(const xdccd::IRCMessage &msg);
        void on_join(const std::string &channel);
        void on_part(const std::string &channel);
        void on_privmsg(const xdccd::IRCMessage &msg);
        void request_file(const std::string &nick, const std::string &slot);
        const std::vector<std::string> &get_channels() const;
        const std::string &get_nickname() const;
        const std::string &get_host() const;
        const std::string &get_port() const;
        const std::vector<DCCFilePtr> &get_files() const;
        const std::map<std::string, DCCAnnouncePtr> &get_announces() const;
        void find_announces(const std::string &query, std::vector<DCCAnnouncePtr> &result) const;
        bot_id_t get_id() const;

    private:
        void add_announce(const std::string &bot, const std::string &filename, const std::string &size, const std::string &slot, const std::string &download_count);
        DCCAnnouncePtr get_announce(const std::string &hash) const;

        bot_id_t id;
        file_id_t last_file_id;
        std::string nickname;
        xdccd::IRCConnection connection;
        std::vector<DCCFilePtr> files;
        std::mutex files_lock;
        ThreadpoolPtr threadpool;
        std::vector<std::string> channels;
        std::vector<std::string> channels_to_join;
        std::map<std::string, DCCAnnouncePtr> announces;
};

typedef std::shared_ptr<DCCBot> DCCBotPtr;

}
