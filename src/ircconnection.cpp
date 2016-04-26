#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>

#include "ircconnection.h"

xdccd::IRCConnection::IRCConnection(const std::string &host, std::string port, const read_handler_t &read_handler, const connected_handler_t &connected_handler, bool use_ssl)
    : host(host),
    port(port),
    use_ssl(use_ssl),
    state(connection::IDLE),
    resolver(io_service),
    work(std::make_unique<boost::asio::io_service::work>(io_service)),
    read_handler(read_handler),
    connected_handler(connected_handler),
    bytes_read(0),
    bytes_written(0),
    reconnect_delay(0),
    timeout_timer(io_service),
    reconnect_timer(io_service),
    message_received(false),
    timeout_triggered(false)
{
}

void xdccd::IRCConnection::connect()
{
    if (use_ssl)
        socket = std::make_unique<xdccd::SSLSocket>(io_service);
    else
        socket = std::make_unique<xdccd::PlainSocket>(io_service);

    boost::asio::ip::tcp::resolver::query query(host, port);

    state = connection::CONNECTING;

    BOOST_LOG_TRIVIAL(debug) << "Trying to resolve " << host << "...";
    resolver.async_resolve(query,
        boost::bind(&xdccd::IRCConnection::on_resolved, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::iterator));
}

void xdccd::IRCConnection::on_resolved(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
    decltype(endpoint_iterator) end;
    boost::system::error_code error = boost::asio::error::host_not_found;

    if (err)
    {
        BOOST_LOG_TRIVIAL(error) << "Error resolving " << host << ": " << err.message();
        start_reconnect_timer();
        return;
    }

    while (endpoint_iterator != end)
    {
        if (!error)
            break;

        socket->close();
        BOOST_LOG_TRIVIAL(info) << "Trying to connect to " << host << ":" << port << " (" << (*endpoint_iterator).endpoint().address().to_string() << ") ...";
        socket->connect(*endpoint_iterator++, error);
    }

    if (error)
    {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to " << host << ": " << error.message();
        start_reconnect_timer();
        return;
    }

    reconnect_delay = xdccd::connection::MIN_RECONNECT_DELAY;

    // Inform the bot about the succesful connection
    connected_handler();

    state = connection::CONNECTED;

    // Start the async read loop
    socket->async_read_until(
            msg_buffer,
            "\r\n",
            boost::bind(&IRCConnection::read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
    );
}

void xdccd::IRCConnection::run()
{
    connect();

    while (!io_service.stopped() && io_service.run_one())
    {
        std::lock_guard<std::mutex> lock(io_lock);

        if (message_received)
        {
            timeout_timer.cancel();

            timeout_lock.lock();
            message_received = false;

            // Restart timeout timer
            timeout_timer.expires_from_now(xdccd::connection::READ_TIMEOUT);
            timeout_timer.async_wait([this](const boost::system::error_code& error){ if (!error) { std::lock_guard<std::mutex> lock(timeout_lock); timeout_triggered = true; } });
            timeout_lock.unlock();
        }
        else if (timeout_triggered)
        {
            socket->close();

            timeout_lock.lock();
            timeout_triggered = false;
            timeout_lock.unlock();

            start_reconnect_timer();
        }
    }
}

void xdccd::IRCConnection::read(const boost::system::error_code& error, std::size_t count)
{
    if (!error)
    {
        timeout_lock.lock();
        message_received = true;
        timeout_lock.unlock();

        bytes_read += count;

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
    else
    {
        BOOST_LOG_TRIVIAL(error) << "Read error: " << error.message();
        start_reconnect_timer();
    }
}

void xdccd::IRCConnection::write(const std::string &message)
{
    socket->async_write(boost::asio::buffer(message + "\r\n"),
            [this](const boost::system::error_code &error, std::size_t bytes_transferred)
            {
                if (error)
                {
                    BOOST_LOG_TRIVIAL(error) << "Write error: " << error.message();
                    start_reconnect_timer();
                    return;
                }

                bytes_written += bytes_transferred;
            });
}

void xdccd::IRCConnection::close()
{
    std::lock_guard<std::mutex> lock(io_lock);

    std::lock_guard<std::mutex> time_lock(timeout_lock);
    message_received = false;
    timeout_triggered = false;
    timeout_timer.cancel();
    reconnect_timer.cancel();

    socket->close();
    io_service.stop();
}

void xdccd::IRCConnection::start_reconnect_timer()
{
    if (reconnect_timer.expires_from_now() > std::chrono::milliseconds::zero())
        return;


    std::lock_guard<std::mutex> lock(timeout_lock);
    reconnect_delay = reconnect_delay == std::chrono::milliseconds::zero() ? xdccd::connection::MIN_RECONNECT_DELAY : reconnect_delay * 2;

    BOOST_LOG_TRIVIAL(info) << "Trying to reconnect again in " << reconnect_delay.count() << "ms ...";

    reconnect_timer.expires_from_now(reconnect_delay);
    reconnect_timer.async_wait([this](const boost::system::error_code& error){ if (!error) { BOOST_LOG_TRIVIAL(info) << "Reconnecting ..."; this->connect(); } else { BOOST_LOG_TRIVIAL(info) << "Error reconnecting!"; } });
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

xdccd::connection::STATE xdccd::IRCConnection::get_state() const
{
    return state;
}

