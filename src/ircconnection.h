#pragma once

#include <functional>
#include <boost/asio.hpp>

#include "socket.h"

namespace xdccd
{

typedef std::function<void (const std::string&)> read_handler_t;
typedef std::function<void (void)> write_handler_t;

class IRCConnection
{
    public:
        IRCConnection(const std::string &host, std::string port, const read_handler_t &read_handler, bool use_ssl);
        bool connect();
        void run();
        void read(const boost::system::error_code& error, std::size_t count);
        void write(const std::string &message);
        void close();
        void set_read_handler(const read_handler_t &handler);
        void set_write_handler(const write_handler_t &handler);
        const std::string &get_host() const;
        const std::string &get_port() const;

    private:
        std::string host;
        std::string port;

        boost::asio::io_service io_service;
        std::unique_ptr<xdccd::Socket> socket;

        read_handler_t read_handler;
        write_handler_t write_handler;

        boost::asio::streambuf msg_buffer;
        std::string partial_msg;
};

}
