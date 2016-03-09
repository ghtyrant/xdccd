#include <iostream>
#include <functional>
#include <boost/bind.hpp>

#include "socket.h"

xdccd::PlainSocket::PlainSocket(boost::asio::io_service &io_service)
    : socket(io_service)
{}

void xdccd::PlainSocket::connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error)
{
    socket.connect(endpoint, error);
}

void xdccd::PlainSocket::close()
{
    socket.close();
}

void xdccd::PlainSocket::cancel()
{
    socket.cancel();
}

void xdccd::PlainSocket::async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle)
{
    boost::asio::async_read_until(socket, b, delim, handle);
}

void xdccd::PlainSocket::async_write(const boost::asio::const_buffers_1 &b, std::function<void(const boost::system::error_code&, std::size_t)> handle)
{
    boost::asio::async_write(socket, b, handle);
}

std::string xdccd::PlainSocket::get_address() const
{
    return socket.local_endpoint().address().to_string();
}

xdccd::SSLSocket::SSLSocket(boost::asio::io_service &io_service)
    : ssl_context(boost::asio::ssl::context::sslv23), socket(io_service, ssl_context)
{
    socket.set_verify_mode(boost::asio::ssl::verify_peer);
    socket.set_verify_callback(boost::bind(&SSLSocket::verify_certificate, this, _1, _2));
}

void xdccd::SSLSocket::connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error)
{
    socket.lowest_layer().connect(endpoint, error);
    socket.handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::client);
}

void xdccd::SSLSocket::close()
{
    socket.lowest_layer().close();
}

void xdccd::SSLSocket::cancel()
{
    socket.lowest_layer().cancel();
}

void xdccd::SSLSocket::async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle)
{
    boost::asio::async_read_until(socket, b, delim, handle);
}

bool xdccd::SSLSocket::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
    std::cout << "Verifiying certificate ... (preverified: " << preverified << ")" << std::endl;
    return true;
}

void xdccd::SSLSocket::async_write(const boost::asio::const_buffers_1 &b, std::function<void(const boost::system::error_code&, std::size_t)> handle)
{
    boost::asio::async_write(socket, b, handle);
}

std::string xdccd::SSLSocket::get_address() const
{
    return socket.lowest_layer().local_endpoint().address().to_string();
}
