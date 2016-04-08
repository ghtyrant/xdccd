#pragma once

#include <functional>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>

#include "socket.h"

namespace xdccd
{

typedef std::function<void (const std::string&)> read_handler_t;
typedef std::function<void (void)> write_handler_t;
typedef std::function<void (void)> connected_handler_t;

namespace connection_limits
{
static const std::chrono::milliseconds MIN_RECONNECT_DELAY(500);
static const std::chrono::seconds MAX_RECONNECT_DELAY(120);
static const std::chrono::seconds READ_TIMEOUT(120);
}

class IRCConnection
{
    public:
        IRCConnection(const std::string &host, std::string port, const read_handler_t &read_handler, const connected_handler_t &connected_handler, bool use_ssl);
        void connect();
        void on_resolved(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
        void run();
        void read(const boost::system::error_code& error, std::size_t count);
        void write(const std::string &message);
        void close();
        void start_reconnect_timer();
        void set_read_handler(const read_handler_t &handler);
        void set_write_handler(const write_handler_t &handler);
        const std::string &get_host() const;
        const std::string &get_port() const;
        std::string get_local_ip() const;

    private:
        std::string host;
        std::string port;
        bool use_ssl;
        bool connected;

        std::mutex io_lock;
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver;
        std::unique_ptr<boost::asio::io_service::work> work;
        std::unique_ptr<xdccd::Socket> socket;

        read_handler_t read_handler;
        write_handler_t write_handler;
        connected_handler_t connected_handler;

        boost::asio::streambuf msg_buffer;
        std::string partial_msg;

        std::chrono::milliseconds reconnect_delay;
        boost::asio::system_timer timeout_timer;
        boost::asio::system_timer reconnect_timer;
        bool message_received;
        bool timeout_triggered;
        std::mutex timeout_lock;
};

}
