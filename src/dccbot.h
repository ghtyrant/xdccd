#pragma once

#include <mutex>
#include <map>
#include <regex>

#include "ircconnection.h"
#include "threadmanager.h"
#include "logable.h"
#include "logging.h"
#include "downloadmanager.h"

namespace xdccd
{

namespace regex
{
    const static std::regex ANNOUNCE("#(\\d{1,3})\\s+(\\d+)x\\s+\\[\\s?(\\d+(?:\\.\\d+)?[KMG])\\] (.*)", std::regex_constants::ECMAScript);
}
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
    std::size_t num_size;

    bool compare(const std::string &other) const;
};

typedef std::shared_ptr<DCCAnnounce> DCCAnnouncePtr;

class DCCRequest
{
    public:
        DCCRequest(DCCAnnouncePtr announce, bool stream);
        DCCAnnouncePtr announce;
        bool stream;
};

typedef std::unique_ptr<DCCRequest> DCCRequestPtr;

class DCCBot : public Logable<DCCBot>
{
    public:
        DCCBot(bot_id_t id, const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl, DownloadManager &download_manager);
        virtual ~DCCBot();
        void read_handler(const std::string &message);

        void run();
        void stop();

        void on_connected();
        void on_welcome();
        void on_ctcp(const xdccd::IRCMessage &msg);
        void on_join(const std::string &channel);
        void on_part(const std::string &channel);
        void on_privmsg(const xdccd::IRCMessage &msg);

        void request_file(const std::string &nick, const std::string &slot, bool stream);

        bot_id_t get_id() const;
        const std::vector<std::string> &get_channels() const;
        const std::string &get_nickname() const;
        const std::string &get_host() const;
        const std::string &get_port() const;
        connection::STATE get_connection_state() const;
        file_size_t get_total_announces_size() const;
        virtual std::string to_string() const;

        const std::map<std::string, DCCAnnouncePtr> &get_announces() const;
        void find_announces(const std::string &query, std::vector<DCCAnnouncePtr> &result) const;

        void change_nick(const std::string &nick);

        static xdccd::logger_type_t logger;

    private:
        void add_announce(const std::string &bot, const std::string &filename, const std::string &size, const std::string &slot, const std::string &download_count);
        DCCAnnouncePtr get_announce(const std::string &hash) const;

        bot_id_t id;
        std::string nickname;

        IRCConnection connection;
        DownloadManager &download_manager;

        std::vector<std::string> channels;
        std::vector<std::string> channels_to_join;
        std::map<std::string, DCCAnnouncePtr> announces;
        file_size_t total_announces_size;

        std::multimap<std::string, DCCRequestPtr> requests;

        std::regex announce_regex;
};

typedef std::shared_ptr<DCCBot> DCCBotPtr;

}
