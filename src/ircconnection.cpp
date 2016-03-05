#include <iostream>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>

#include "ircconnection.h"

xdccd::IRCConnection::IRCConnection(const std::string &host, std::string port, const read_handler_t &read_handler, bool use_ssl)
    : host(host), port(port), read_handler(read_handler)
{
    if (use_ssl)
        socket = std::make_unique<xdccd::SSLSocket>(io_service);
    else
        socket = std::make_unique<xdccd::PlainSocket>(io_service);

    connect();
}

xdccd::IRCConnection::~IRCConnection()
{
    BOOST_LOG_TRIVIAL(warning) << "IRCConnection::~IRCConnection()";
    close();
    while (!io_service.stopped())
    {}
}

bool xdccd::IRCConnection::connect()
{
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(host, port);
    boost::system::error_code error = boost::asio::error::host_not_found;

    auto iter = resolver.resolve(query);
    decltype(iter) end;

    while (iter != end)
    {
        if (!error)
            break;

        std::cout << "Connecting ..." << std::endl;
        socket->close();
        socket->connect(*iter++, error);

        if (error)
            std::cout << "Error connecting to " << host << ": " << error.message() << std::endl;
    }

    if (error)
    {
        std::cout << "Error connecting to " << host << ": " << error.message() << std::endl;
        return false;
    }

    return true;
}

void xdccd::IRCConnection::run()
{
    socket->async_read_until(
            msg_buffer,
            "\r\n",
            boost::bind(&IRCConnection::read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
    );

    io_service.run();
}

void xdccd::IRCConnection::read(const boost::system::error_code& error, std::size_t count)
{
    if (error)
        close();
    else
    {
        boost::asio::streambuf::const_buffers_type bufs = msg_buffer.data();
        read_handler(
                std::string(boost::asio::buffers_begin(bufs),
                            boost::asio::buffers_begin(bufs) + count - 2)
                );

        msg_buffer.consume(count);

        socket->async_read_until(
            msg_buffer,
            "\r\n",
            boost::bind(&IRCConnection::read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
        );
    }
}

void xdccd::IRCConnection::write(const std::string &message)
{
    socket->write(boost::asio::buffer(message + "\r\n"));
}

void xdccd::IRCConnection::close()
{
    BOOST_LOG_TRIVIAL(warning) << "IRCConnection::close()";
    socket->close();

    if (!io_service.stopped())
        io_service.stop();
}

void xdccd::IRCConnection::set_read_handler(const read_handler_t &handler)
{
    read_handler = handler;
}

void xdccd::IRCConnection::set_write_handler(const write_handler_t &handler)
{
    write_handler = handler;
}

const std::string &xdccd::IRCConnection::get_host() const
{
    return host;
}

const std::string &xdccd::IRCConnection::get_port() const
{
    return port;
}

std::string xdccd::IRCConnection::get_local_ip() const
{
    return socket->get_address();
}

