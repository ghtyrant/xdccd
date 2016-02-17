#include <iostream>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <cinttypes>
#include <thread>
#include <chrono>

#include "threadpool.h"

using namespace std::literals;

typedef std::function<void (const std::string&)> read_handler_t;
typedef std::function<void (void)> write_handler_t;

class IRCMessage
{
    public:
        IRCMessage(const std::string &msg)
            : ctcp(false), raw(msg)
        {
            std::istringstream iss(msg);
            iss >> from >> type >> target;

            // If there's something left in the message, it's most likely
            // data starting with ":". So we ignore the leading space and
            // the following double-colon
            if (!iss.eof())
            {
                iss.ignore(2);
                std::getline(iss, message);
            }

            if (type.compare("PRIVMSG") == 0 && message[0] == 0x01 && message[message.length()-1] == 0x01)
            {
                ctcp = true;
                message = message.substr(1, message.length() - 2);
                std::istringstream iss(message);
                iss >> ctcp_type;

                if (!iss.eof())
                {
                    iss.ignore(1);
                    std::getline(iss, ctcp_message);
                }
            }
        }
        
        std::string from;
        std::string type;
        std::string target;
        std::string message;
        bool ctcp;
        std::string ctcp_type;
        std::string ctcp_message;

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
                boost::asio::streambuf::const_buffers_type bufs = msg_buffer.data();
                read_handler(
                        std::string(boost::asio::buffers_begin(bufs),
                                    boost::asio::buffers_begin(bufs) + count - 2)
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

class DCCFile
{
    public:
        DCCFile(const std::string &ip, const std::string &port, const std::string &filename, std::uintmax_t size)
            : ip(ip), port(port), filename(filename), size(size), transfer_started(false)
        {}

        std::string ip;
        std::string port;
        std::string filename;
        std::uintmax_t size;
        bool transfer_started;
};

class Task
{};

class DCCReceiveTask : public Task
{
    public:
        DCCReceiveTask(std::shared_ptr<DCCFile> file)
            : file(file), socket(io_service)
        {}

        bool connect()
        {
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(file->ip, file->port);
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
                    std::cout << "Error connecting to " << file->ip << ": " << error.message() << std::endl;
            }

            if (error)
                return false;

            return true;
        }

        void operator()()
        {
            if (!connect())
                return;

            std::cout << "Start download!" << std::endl;

            for (;;)
            {
                std::array<char, 4096> buffer;
                boost::system::error_code error;

                std::size_t len = socket.read_some(boost::asio::buffer(buffer), error);
                std::cout << "Read " << len << "bytes!" << std::endl;

                if (error == boost::asio::error::eof)
                    break; // Connection closed cleanly by peer.
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

            };

            std::cout << "Downloaded file!" << std::endl;
        }

    private:
        std::shared_ptr<DCCFile> file;
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::socket socket;
};

class DCCBot
{
    public:
        DCCBot(const std::string &host, const std::string &port, const std::string &nick)
            : connection(host, port, ([this](const std::string &msg) { this->read_handler(msg); })),
            threadpool(5)
        {
            connection.write((boost::format("NICK %s") % nick).str());
            connection.write((boost::format("USER %s * * %s") % nick % nick).str());
        } 

        void read_handler(const std::string &message)
        {
            IRCMessage msg(message);

            std::cout << "Received: '" << message << "' (CTCP: " << msg.ctcp << ")" << std::endl;

            if (msg.raw.substr(0, 4) == "PING")
            {
                connection.write("PONG " + msg.raw.substr(5));
                return;
            }

            if (msg.type == "001")
                on_connected();

            if (msg.ctcp)
                handle_ctcp(msg);
        }

        void handle_ctcp(const IRCMessage &msg)
        {
            std::cout << "CTCP-Message: " << msg.ctcp_type << " says: '" << msg.ctcp_message << "'" << std::endl;

            if (msg.ctcp_type.compare("DCC") == 0)
            {
                std::istringstream iss(msg.ctcp_message);
                std::string cmd;
                iss >> cmd;

                if (cmd.compare("SEND") == 0)
                {
                    std::string filename;
                    std::string ip;
                    std::string port;
                    std::string size;
                    iss >> filename >> ip >> port >> size;

                    unsigned long int_ip = std::stol(ip);
                    boost::asio::ip::address_v4 addr(int_ip);

                    std::cout << "DCC SEND request, offering file " << filename << " on ip " << addr.to_string() << ":" << port << std::endl;

                    std::lock_guard<std::mutex> lock(files_lock);
                    std::shared_ptr<DCCFile> file(std::make_shared<DCCFile>(ip, port, filename, std::strtoumax(size.c_str(), nullptr, 10)));
                    files.push_back(file);

                    threadpool.run_task(DCCReceiveTask(file));
                }
            }
                
        }

        void run()
        {
            connection.run();
        }

        void on_connected()
        {
            connection.write("JOIN #asdfasdf");
        }

    private:
        IRCConnection connection;
        std::vector<std::shared_ptr<DCCFile>> files;
        std::mutex files_lock;
        Threadpool threadpool;
};

int main(int argc, char* argv[])
{

    //DCCBot bot("irc.abjects.net", "6667", "test123");
    DCCBot bot("blitzforum.de", "6667", "test123");
    bot.run();

    std::cout << "Quitting ..." << std::endl;
    return 0;
}
