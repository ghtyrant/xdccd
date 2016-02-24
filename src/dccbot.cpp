#include <boost/format.hpp>
#include <cinttypes>

#include "dccbot.h"
#include "dccreceivetask.h"
#include "ircmessage.h"

xdccd::DCCBot::DCCBot(ThreadpoolPtr threadpool, const std::string &host, const std::string &port, const std::string &nick, bool use_ssl)
    : nickname(nick), connection(host, port, ([this](const std::string &msg) { this->read_handler(msg); }), use_ssl),
    threadpool(threadpool)
{
    connection.write((boost::format("NICK %s") % nick).str());
    connection.write((boost::format("USER %s * * %s") % nick % nick).str());
} 

void xdccd::DCCBot::read_handler(const std::string &message)
{
    xdccd::IRCMessage msg(message);

    if (msg.command != "PRIVMSG")
        std::cout << "Received: '" << message << "' (CTCP: " << msg.ctcp << ")" << std::endl;

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

    if (msg.ctcp)
    {
        on_ctcp(msg);
        return;
    }
}

void xdccd::DCCBot::on_ctcp(const xdccd::IRCMessage &msg)
{
    std::cout << "CTCP-Message: " << msg.ctcp_command << " says: '" << msg.params[1] << "'" << std::endl;

    if (msg.ctcp_command == "DCC")
    {
        if (msg.ctcp_params[0] == "SEND")
        {
            std::string filename = msg.ctcp_params[1];
            std::string ip = msg.ctcp_params[2];
            std::string port = msg.ctcp_params[3];
            std::string size = msg.ctcp_params[4];

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

            std::cout << "DCC SEND request, offering file " << filename << " on ip " << addr.to_string() << ":" << port << std::endl;

            std::lock_guard<std::mutex> lock(files_lock);
            std::shared_ptr<DCCFile> file(std::make_shared<DCCFile>(addr.to_string(), port, filename, std::strtoumax(size.c_str(), nullptr, 10)));
            files.push_back(file);

            threadpool->run_task(DCCReceiveTask(file));
        }
    }
}

void xdccd::DCCBot::run()
{
    connection.run();
}

void xdccd::DCCBot::on_connected()
{
    connection.write("JOIN #mg-chat");
    connection.write("JOIN #moviegods");
}

void xdccd::DCCBot::on_join(const std::string &channel)
{
    channels.push_back(channel);
}

void xdccd::DCCBot::on_part(const std::string &channel)
{
    std::remove_if(channels.begin(), channels.end(), [channel](const std::string &name) { return name == channel; });
}

const std::vector<std::string> xdccd::DCCBot::get_channels() const
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

