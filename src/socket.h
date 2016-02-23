#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

class Socket
{
    public:
        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error) = 0;
        virtual void close() = 0;
        virtual boost::asio::ip::tcp::socket &get_socket() = 0;
        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, boost::function<void(const boost::system::error_code&, std::size_t)> handle) = 0;
        virtual void write(const boost::asio::const_buffers_1 &b) = 0;
};

class PlainSocket : public Socket
{
    public:

        PlainSocket(boost::asio::io_service &io_service)
            : socket(io_service)
        {}

        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error)
        {
            socket.connect(endpoint, error);
        }

        virtual void close()
        {
            socket.close();
        }

        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, boost::function<void(const boost::system::error_code&, std::size_t)> handle)
        {
            boost::asio::async_read_until(socket, b, delim, handle);
        }

        virtual boost::asio::ip::tcp::socket &get_socket()
        {
            return socket;
        }

        virtual void write(const boost::asio::const_buffers_1 &b)
        {
            boost::asio::write(socket, b);
        }

    private:
        boost::asio::ip::tcp::socket socket;
};

class SSLSocket : public Socket
{
    public:

        SSLSocket(boost::asio::io_service &io_service)
            : ssl_context(boost::asio::ssl::context::sslv23), socket(io_service, ssl_context)
        {
            socket.set_verify_mode(boost::asio::ssl::verify_peer);
            socket.set_verify_callback(boost::bind(&SSLSocket::verify_certificate, this, _1, _2));
        }

        virtual void connect(const boost::asio::ip::tcp::endpoint& endpoint, boost::system::error_code& error)
        {
            socket.lowest_layer().connect(endpoint, error);
            socket.handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::client);
        }

        virtual void close()
        {
            socket.lowest_layer().close();
        }

        virtual void async_read_until(boost::asio::streambuf& b, const std::string& delim, boost::function<void(const boost::system::error_code&, std::size_t)> handle)
        {
            boost::asio::async_read_until(socket, b, delim, handle);
        }

        virtual boost::asio::ip::tcp::socket &get_socket()
        {
        }

    private:

        bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
        {
            std::cout << "Verifiying certificate ... (preverified: " << preverified << ")" << std::endl;
            return true;
        }

        virtual void write(const boost::asio::const_buffers_1 &b)
        {
            boost::asio::write(socket, b);
        }

        boost::asio::ssl::context ssl_context;
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
};
