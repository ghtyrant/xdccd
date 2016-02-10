#include <iostream>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

typedef std::function<void (const std::string&)> read_handler_t;
typedef std::function<void (void)> write_handler_t;

class IRCMessage
{
    public:
        IRCMessage(const std::string &msg)
            : raw(msg)
        {
            std::istringstream iss(msg);
            iss >> from >> type >> target >> message;
        }
        
        std::string from;
        std::string type;
        std::string target;
        std::string message;

        std::string raw;
};

class IRCConnection
{
    public:
        IRCConnection(const std::string &host, std::string port, const read_handler_t &read_handler)
            : host(host), port(port), socket(io_service), read_handler(read_handler)
        {
            connect();
        }

        bool connect()
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

                socket.close();
                socket.connect(*iter++, error);

                if (error)
                    std::cout << "Error connecting to " << host << ": " << error.message() << std::endl;
            }

            if (error)
                return false;

            return true;
        }

        void run()
        {
            boost::asio::async_read_until(
                    socket,
                    msg_buffer,
                    "\r\n",
                    boost::bind(&IRCConnection::read, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
            );

            io_service.run();
        }

        void read(const boost::system::error_code& error, std::size_t count)
        {
            if (error)
                close();
            else
            {
                /*std::size_t start = 0;
                for (std::size_t i = 0; i < count; i++)
                {
                    if (msg_buffer[i] == '\r' && (i < count - 1 && msg_buffer[i+1] == '\n'))
                    {
                        read_handler(partial_msg + std::string(msg_buffer.data(), start, i - start));
                        partial_msg = "";
                        start = i+2;
                    }
                }

                if (start != count)
                    partial_msg = std::string(msg_buffer.data(), start, count - start);
                */

                read_handler(
                        std::string(boost::asio::buffers_begin(msg_buffer.data()),
                                    boost::asio::buffers_begin(msg_buffer.data()) + count)
                        );

                msg_buffer.consume(count);

                boost::asio::async_read_until(
                    socket,
                    msg_buffer,
                    "\r\n",
                    boost::bind(&IRCConnection::read, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
                );
            }
        }

        void write(const std::string &message)
        {
            boost::asio::write(socket, boost::asio::buffer(message + "\r\n"));
        }

        void close()
        {
            socket.close();
            io_service.stop();
        }

        void set_read_handler(const read_handler_t &handler)
        {
            read_handler = handler;
        }

        void set_write_handler(const write_handler_t &handler)
        {
            write_handler = handler;
        }


    private:
        std::string host;
        std::string port;

        boost::asio::io_service io_service;
        boost::asio::ip::tcp::socket socket;

        read_handler_t read_handler;
        write_handler_t write_handler;

        boost::asio::streambuf msg_buffer;
        std::string partial_msg;
};

class DCCBot
{
    public:
        DCCBot(const std::string &host, const std::string &port)
            : connection(host, port, ([this](const std::string &msg) { this->read_handler(msg); }))
        {
            connection.write("NICK test\r\nUSER test * * :test");
        } 

        void read_handler(const std::string &message)
        {
            IRCMessage msg(message);

            std::cout << "Received: " << message << std::endl;

            if (msg.raw.substr(0, 4) == "PING")
            {
                connection.write("PONG " + msg.raw.substr(5));
                return;
            }

            if (msg.type == "001")
                on_connected();
        }

        void run()
        {
            connection.run();
        }

        void on_connected()
        {
            connection.write("JOIN #moviegods");
        }

    private:
        IRCConnection connection;
};

int main(int argc, char* argv[])
{

    DCCBot bot("irc.abjects.net", "6667");
    bot.run();

    std::cout << "Quitting ..." << std::endl;
    return 0;
}
