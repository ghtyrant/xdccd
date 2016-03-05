#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/log/trivial.hpp>

namespace xdccd
{
class Socket
{
    public:
        virtual ~Socket() { BOOST_LOG_TRIVIAL(warning) << "Socket::~Socket()"; }
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error) = 0;
        virtual void close() = 0;
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle) = 0;
        virtual void write(const boost::asio::const_buffers_1 &b) = 0;
        virtual std::string get_address() const = 0;
};

class PlainSocket : public Socket
{
    public:
        PlainSocket(boost::asio::io_service &io_service);
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error);
        virtual void close();
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle);
        virtual void write(const boost::asio::const_buffers_1 &b);
        virtual std::string get_address() const;

    private:
        boost::asio::ip::tcp::socket socket;
};

class SSLSocket : public Socket
{
    public:
        SSLSocket(boost::asio::io_service &io_service);
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error);
        virtual void close();
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle);
        virtual void write(const boost::asio::const_buffers_1 &b);
        virtual std::string get_address() const;

    private:
        bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

        boost::asio::ssl::context ssl_context;
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
};
}
