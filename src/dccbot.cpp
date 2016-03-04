#include <boost/format.hpp>
#include <cinttypes>
#include <regex>

#include "dccbot.h"
#include "dccreceivetask.h"
#include "ircmessage.h"


xdccd::DCCAnnounce::DCCAnnounce(
        const std::string &bot,
        const std::string &filename,
        const std::string &size,
        const std::string &slot,
        const std::string &download_count)
    : hash(bot+slot), bot(bot), filename(filename), size(size), slot(slot), download_count(download_count)
{}

bool xdccd::DCCAnnounce::compare(const std::string &other) const
{
    return boost::algorithm::contains(filename, other);
}

xdccd::DCCBot::DCCBot(bot_id_t id, ThreadpoolPtr threadpool, const std::string &host, const std::string &port, const std::string &nick, const std::vector<std::string> &channels, bool use_ssl)
    : id(id), last_file_id(0), nickname(nick), connection(host, port, ([this](const std::string &msg) { this->read_handler(msg); }), use_ssl),
    threadpool(threadpool), channels_to_join(channels)
{
    std::string result = boost::algorithm::join(channels, ", ");
    std::cout << "Started bot for '" << host << ":" << port << "', called '" << nick  << "', joining: " << result << std::endl;
    connection.write((boost::format("NICK %s") % nick).str());
    connection.write((boost::format("USER %s * * %s") % nick % nick).str());
}

xdccd::DCCBot::~DCCBot()
{
    std::cout << "Deleting bot '" << nickname << "'!" << std::endl;
}

void xdccd::DCCBot::read_handler(const std::string &message)
{
    xdccd::IRCMessage msg(message);

    if (msg.command == "PING")
    {
        connection.write("PONG " + msg.params[0]);
        return;
    }

    // 001 is the server's welcome message
    if (msg.command == "001")
    {
        on_connected();
        return;
    }

    // 366 is the end of a channel's name list
    if (msg.command == "366")
    {
        on_join(msg.params[1]);
        return;
    }

    if (msg.command == "PART" && msg.params[0] == nickname)
    {
        on_part(msg.params[1]);
        return;
    }

    if (msg.command == "PRIVMSG")
    {
        if (msg.ctcp)
        {
            on_ctcp(msg);
            return;
        }

        on_privmsg(msg);
    }
}

void xdccd::DCCBot::on_ctcp(const xdccd::IRCMessage &msg)
{
    std::cout << "CTCP-Message: " << msg.ctcp_command << " says: '" << msg.params[1] << "'" << std::endl;

    if (msg.ctcp_command == "DCC")
    {
        if (msg.ctcp_params[0] == "SEND")
        {
            bool active = true;
            std::string filename = msg.ctcp_params[1];
            std::string ip = msg.ctcp_params[2];
            std::string port = msg.ctcp_params[3];
            std::string size = msg.ctcp_params[4];

            // Check if we got an IPv6 or v4 address
            boost::asio::ip::address addr;
            if (ip.find(':') != std::string::npos)
            {
                boost::asio::ip::address_v6 tmp(boost::asio::ip::address_v6::from_string(ip));
                addr = boost::asio::ip::address(tmp);
            }
            else
            {
                unsigned long int_ip = std::stol(ip);
                boost::asio::ip::address_v4 tmp(int_ip);
                addr = boost::asio::ip::address(tmp);
            }

            // Other side wants to initiate passive DCC
            if (port == "0")
            {
                active = false;

                // We have to send a DCC SEND request back, containing our IP address
                // DCC SEND <filename> <ip> <port> <filesize> <token> 
                connection.write((boost::format("PRIVMSG %s :" "\x01" "DCC SEND %s %s %s %s\x01") 
                            % msg.nickname
                            % filename
                            % connection.get_local_ip()
                            % "12345"
                            % size).str());
            }

            std::cout << "DCC SEND request, offering file " << filename << " on ip " << addr.to_string() << ":" << port << std::endl;

            std::lock_guard<std::mutex> lock(files_lock);
            std::shared_ptr<DCCFile> file_ptr = nullptr;

            // Check if we are expecting the incoming file
            for(auto file : files)
                if (file->bot == msg.nickname && file->filename == filename && file->state == xdccd::FileState::AWAITING_CONNECTION)
                    file_ptr = file;

            // If not, add it anyway and wait for user interaction
            if (file_ptr == nullptr)
            {
                file_ptr = std::make_shared<DCCFile>(last_file_id++, msg.nickname, addr.to_string(), port, filename, std::strtoumax(size.c_str(), nullptr, 10));
                file_ptr->state = xdccd::FileState::AWAITING_RESPONSE;
                files.push_back(file_ptr);
            }

            threadpool->run_task(DCCReceiveTask(file_ptr, active));
        }
    }
}

void xdccd::DCCBot::run()
{
    connection.run();
}

void xdccd::DCCBot::stop()
{
    std::cout << "Disconnecting bot '" << nickname << "'!" << std::endl;
    connection.close();
}

void xdccd::DCCBot::on_connected()
{
    std::cout << "Connected!" << std::endl;

    for (auto channel_name : channels_to_join)
        connection.write("JOIN " + channel_name);

    channels_to_join.clear();
}

void xdccd::DCCBot::on_join(const std::string &channel)
{
    std::cout << "Joined channel " << channel << std::endl;
    channels.push_back(channel);
}

void xdccd::DCCBot::on_part(const std::string &channel)
{
    std::remove_if(channels.begin(), channels.end(), [channel](const std::string &name) { return name == channel; });
}

void xdccd::DCCBot::on_privmsg(const xdccd::IRCMessage &msg)
{
    static std::regex announce("#(\\d{1,3})\\s+(\\d+)x\\s+\\[\\s?(\\d+(?:\\.\\d+)?M|G)\\] (.*)", std::regex_constants::ECMAScript);

    std::smatch m;
    if (!std::regex_match(msg.params[1], m, announce))
        return;

    if (!m.empty() && m.size() == 5)
    {
        std::vector<std::string> result;
        for (std::size_t i = 1; i < m.size(); ++i)
            result.push_back(m[i].str());

        add_announce(msg.nickname, result[3], result[2], result[0], result[1]);
    }
}

void xdccd::DCCBot::request_file(const std::string &nick, const std::string &slot)
{
    connection.write((boost::format("PRIVMSG %s :xdcc send #%s") % nick % slot).str());

}

const std::vector<std::string> &xdccd::DCCBot::get_channels() const
{
    return channels;
}

const std::string &xdccd::DCCBot::get_nickname() const
{
    return nickname;
}

const std::string &xdccd::DCCBot::get_host() const
{
    return connection.get_host();
}

const std::string &xdccd::DCCBot::get_port() const
{
    return connection.get_port();
}

void xdccd::DCCBot::add_announce(const std::string &bot, const std::string &filename, const std::string &size, const std::string &slot, const std::string &download_count)
{
    DCCAnnouncePtr announce = std::make_shared<DCCAnnounce>(bot, filename, size, slot, download_count);
    announces[announce->hash] = announce;

    /*std::cout << "[Bot Announce] Bot: " << bot
        << ", File: " << filename
        << ", Size: " << size
        << " in slot #" << slot
        << ", downloaded " << download_count
        << " times (size: " << sizeof(*announce)
        << "b, total size ~ " << sizeof(*announce) * announces.size() + sizeof(announces)
        << "b, announces: " << announces.size()
        << ")" << std::endl;
        */
}

xdccd::DCCAnnouncePtr xdccd::DCCBot::get_announce(const std::string &hash) const
{
    auto it = announces.find(hash);

    if (it == announces.end())
        return nullptr;

    return it->second;
}

void xdccd::DCCBot::find_announces(const std::string &query, std::vector<DCCAnnouncePtr> &result) const
{
    for (auto announce : announces)
    {
        if (announce.second->compare(query))
            result.push_back(announce.second);
    }
}

const std::vector<xdccd::DCCFilePtr> &xdccd::DCCBot::get_files() const
{
    return files;
}

const std::map<std::string, xdccd::DCCAnnouncePtr> &xdccd::DCCBot::get_announces() const
{
    return announces;
}

xdccd::bot_id_t xdccd::DCCBot::get_id() const
{
    return id;
}
