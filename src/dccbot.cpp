#include <cinttypes>
#include <regex>
#include <boost/format.hpp>

#include "dccbot.h"
#include "ircmessage.h"


xdccd::DCCAnnounce::DCCAnnounce(
        xdccd::bot_id_t bot_id,
        const std::string &bot_name,
        const std::string &filename,
        const std::string &size,
        const std::string &slot,
        const std::string &download_count)
    : bot_id(bot_id), hash(bot_name+slot), bot_name(bot_name), filename(filename), size(size), slot(slot), download_count(download_count)
{
    std::string tmp = size;
    std::size_t factor = 1;
    if (tmp[tmp.size()-1] == 'M')
        factor = 1024;
    else if (tmp[tmp.size()-1] == 'G')
        factor = 1024 * 1024;

    tmp[tmp.size()-1] = '\0';
    num_size = std::stoull(tmp) * factor;
}

bool xdccd::DCCAnnounce::compare(const std::string &other) const
{
    return boost::algorithm::icontains(filename, other);
}

xdccd::DCCRequest::DCCRequest(DCCAnnouncePtr announce, bool stream)
    : announce(announce),
    stream(stream)
{
}

xdccd::logger_type_t xdccd::DCCBot::logger(boost::log::keywords::channel = "DCCBot");

xdccd::DCCBot::DCCBot(bot_id_t id,
        const std::string &host,
        const std::string &port,
        const std::string &nick,
        const std::vector<std::string> &channels,
        bool use_ssl,
        DownloadManager &dl_manager)

    : id(id),
    nickname(nick),
    connection(host, port, ([this](const std::string &msg) { this->read_handler(msg); }),([this]() { this->on_connected(); }), use_ssl),
    download_manager(dl_manager),
    channels_to_join(channels),
    total_announces_size(0)
{
    std::string result = boost::algorithm::join(channels, ", ");
    BOOST_LOG_TRIVIAL(info) << "Started " << *this << " for '" << host << ":" << port << "', called '" << nick  << "', auto-joining: " << result;
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
        on_welcome();
        return;
    }

    // 366 is the end of a channel's name list
    if (msg.command == "366")
    {
        on_join(msg.params[1]);
        return;
    }

    // Check if we somehow left the channel
    if (msg.command == "PART" && msg.nickname == nickname)
    {
        on_part(msg.params[0]);
        return;
    }

    // Check if we got kicked from the channel
    if (msg.command == "KICK" && msg.params[1] == nickname)
    {
        on_part(msg.params[0]);
        return;
    }

    // Nickname already in use
    if (msg.command == "433")
    {
        // Just append a random number to the end of the nickname
        std::string new_nick = nickname + std::to_string(std::rand() % 10);
        BOOST_LOG_TRIVIAL(info) << "Nick of " << *this << " is already in use, changing it to '" << new_nick << "'!";
        change_nick(new_nick);
        return;
    }

    // Erronous nickname
    if (msg.command == "432")
    {
        // TODO somehow handle this
    }

    if (msg.command == "PRIVMSG")
    {
        if (msg.ctcp)
        {
            on_ctcp(msg);
            return;
        }

        on_privmsg(msg);

        if (msg.params[0] == nickname)
            BOOST_LOG_TRIVIAL(info) << "Received private message: " << message;
    }

    if (msg.command == "NOTICE")
    {
        BOOST_LOG_TRIVIAL(info) << "Received notice: " << message;
    }
}

void xdccd::DCCBot::on_ctcp(const xdccd::IRCMessage &msg)
{
    BOOST_LOG_TRIVIAL(info) << "CTCP-Message: " << msg.ctcp_command << " says: '" << msg.params[1] << "' (CTCP-Param[0] = '" << msg.ctcp_params[0] << "')";

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
                unsigned long int_ip = std::stoul(ip);
                boost::asio::ip::address_v4 tmp(int_ip);
                addr = boost::asio::ip::address(tmp);
            }

            BOOST_LOG_TRIVIAL(info) << "Incoming DCC SEND request, offering file " << filename << " on ip " << addr.to_string() << ":" << port;

            // Other side wants to initiate passive DCC
            if (port == "0")
            {
                active = false;

                BOOST_LOG_TRIVIAL(info) << "Passive DCC offer, answering with my IP and port.";

                // We have to send a DCC SEND request back, containing our IP address
                // DCC SEND <filename> <ip> <port> <filesize> <token>
                connection.write((boost::format("PRIVMSG %s :" "\x01" "DCC SEND %s %s %s %s\x01")
                            % msg.nickname
                            % filename
                            % connection.get_local_ip()
                            % "12345"
                            % size).str());
            }

            auto request_iter = requests.find(msg.nickname);

            // DCC SEND offer has not been requested by us
            if (request_iter == requests.end())
                return;
            else
            {
                download_manager.start_download(ip, port, filename, std::stoull(size), active, false);//request_iter->second->stream);
                requests.erase(request_iter);
            }
        }
    }
}

void xdccd::DCCBot::run()
{
    BOOST_LOG_TRIVIAL(warning) << "Running IRCConnection ...";
    connection.run();
}

void xdccd::DCCBot::stop()
{
    BOOST_LOG_TRIVIAL(info) << "Disconnecting bot " << *this;
    connection.write("QUIT :Bye");
    connection.close();
}

void xdccd::DCCBot::on_connected()
{
    change_nick(nickname);
    connection.write((boost::format("USER %s * * :%s") % nickname % nickname).str());
}

void xdccd::DCCBot::on_welcome()
{
    BOOST_LOG_TRIVIAL(info) << "Connected bot " << *this;

    // Remove leftover channels
    channels.clear();

    for (auto channel_name : channels_to_join)
        connection.write("JOIN " + channel_name);
}

void xdccd::DCCBot::on_join(const std::string &channel)
{
    BOOST_LOG_TRIVIAL(info) << "Bot " << *this << " joined channel " << channel;
    channels.push_back(channel);
}

void xdccd::DCCBot::on_part(const std::string &channel)
{
    BOOST_LOG_TRIVIAL(info) << "Bot " << *this << " left channel " << channel;
    channels.erase(std::remove_if(channels.begin(), channels.end(), [channel](const std::string &name) { return name == channel; }));
}

void xdccd::DCCBot::on_privmsg(const xdccd::IRCMessage &msg)
{
    static std::regex announce("#(\\d{1,3})\\s+(\\d+)x\\s+\\[\\s?(\\d+(?:\\.\\d+)?[KMG])\\] (.*)", std::regex_constants::ECMAScript);

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

void xdccd::DCCBot::request_file(const std::string &nick, const std::string &slot, bool stream)
{
    BOOST_LOG_TRIVIAL(info) << "Requesting file in slot #" << slot << " from bot '" << nick << "' on " << *this << " (streaming: " << stream << ")";

    connection.write((boost::format("PRIVMSG %s :xdcc send #%s") % nick % slot).str());

    // Check if we already discovered the file the user wants to download
    DCCAnnouncePtr announce = get_announce(nick + slot);

    requests.insert(std::pair<std::string, DCCRequestPtr>(nick, std::make_unique<DCCRequest>(announce, stream)));
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
    DCCAnnouncePtr announce = std::make_shared<DCCAnnounce>(id, bot, filename, size, slot, download_count);
    total_announces_size += announce->num_size;

    auto old_announce = announces.find(announce->hash);
    if (old_announce != announces.end())
    {
        total_announces_size -= old_announce->second->num_size;
        announces.insert(old_announce, std::pair<std::string, DCCAnnouncePtr>(announce->hash, announce));
    }
    else
        announces[announce->hash] = announce;
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

const std::map<std::string, xdccd::DCCAnnouncePtr> &xdccd::DCCBot::get_announces() const
{
    return announces;
}

xdccd::bot_id_t xdccd::DCCBot::get_id() const
{
    return id;
}

void xdccd::DCCBot::change_nick(const std::string &nick)
{
    connection.write((boost::format("NICK %s") % nick).str());
    nickname = nick;
}

std::string xdccd::DCCBot::to_string() const
{
    return "<Bot #" + std::to_string(id) + " '" + nickname + "'>";
}

xdccd::connection::STATE xdccd::DCCBot::get_connection_state() const
{
    return connection.get_state();
}

xdccd::file_size_t xdccd::DCCBot::get_total_announces_size() const
{
    return total_announces_size;
}
