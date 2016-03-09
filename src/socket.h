#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/log/trivial.hpp>

namespace xdccd
{
class Socket
{
    public:
        virtual ~Socket() {};
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error) = 0;
        virtual void close() = 0;
        virtual void cancel() = 0;
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle) = 0;
        virtual void async_write(const boost::asio::const_buffers_1 &b, std::function<void(const boost::system::error_code&, std::size_t)> handle) = 0;
        virtual std::string get_address() const = 0;
        virtual bool is_open() const = 0;
};

class PlainSocket : public Socket
{
    public:
        PlainSocket(boost::asio::io_service &io_service);
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error);
        virtual void close();
        virtual void cancel();
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle);
        virtual void async_write(const boost::asio::const_buffers_1 &b, std::function<void(const boost::system::error_code&, std::size_t)> handle);
        virtual std::string get_address() const;
        virtual bool is_open() const;

    private:
        boost::asio::ip::tcp::socket socket;
};

class SSLSocket : public Socket
{
    public:
        SSLSocket(boost::asio::io_service &io_service);
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error);
        virtual void close();
        virtual void cancel();
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, std::function<void(const boost::system::error_code&, std::size_t)> handle);
        virtual void async_write(const boost::asio::const_buffers_1 &b, std::function<void(const boost::system::error_code&, std::size_t)> handle);
        virtual std::string get_address() const;
        virtual bool is_open() const;

    private:
        bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

        boost::asio::ssl::context ssl_context;
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
};
}
