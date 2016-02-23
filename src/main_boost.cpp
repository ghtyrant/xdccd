#include <iostream>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <cinttypes>
#include <thread>
#include <chrono>

#include "threadpool.h"
#include "socket.h"

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
        IRCConnection(const std::string &host, std::string port, const read_handler_t &read_handler, bool use_ssl)
            : host(host), port(port), read_handler(read_handler)
        {
            std::cout << typeid(msg_buffer).name() << std::endl;
            if (use_ssl)
                socket = std::make_unique<SSLSocket>(io_service);
            else
                socket = std::make_unique<PlainSocket>(io_service);

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

                std::cout << "Connecting ..." << std::endl;
                socket->close();
                socket->connect(*iter++, error);

                if (error)
                    std::cout << "Error connecting to " << host << ": " << error.message() << std::endl;
            }

            if (error)
                return false;

            return true;
        }

        void run()
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

                socket->async_read_until(
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
            socket->write(boost::asio::buffer(message + "\r\n"));
        }

        void close()
        {
            socket->close();
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
        std::unique_ptr<Socket> socket;

        read_handler_t read_handler;
        write_handler_t write_handler;

        boost::asio::streambuf msg_buffer;
        std::string partial_msg;
};

class DCCFile
{
    public:
        DCCFile(const std::string &ip, const std::string &port, const std::string &filename, std::uintmax_t size)
            : ip(ip), port(port), filename(filename), size(size), received(0), transfer_started(false), path("downloads")
        {
            path /= filename;
        }

        void open()
        { 
            stream.open(path, std::ios::binary);
        }

        void close()
        {
            stream.close();
        }

        void write(const char* data, std::streamsize len)
        {
          stream.write(data, len);
        }

        std::string ip;
        std::string port;
        std::string filename;
        std::uintmax_t size;
        std::uintmax_t received;
        bool transfer_started;
        boost::filesystem::path path;
        boost::filesystem::ofstream stream;
};

class DCCReceiveTask
{
    public:
        DCCReceiveTask(std::shared_ptr<DCCFile> file)
            : file(file)
        {}

        bool connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket)
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
            boost::asio::io_service io_service;
            boost::asio::ip::tcp::socket socket(io_service);

            if (!connect(io_service, socket))
                return;

            std::cout << "Start download!" << std::endl;

            float old_percent = 0.0f;
            file->open();
            for (;;)
            {
                std::array<char, 8196> buffer;
                boost::system::error_code error;

                std::size_t len = socket.read_some(boost::asio::buffer(buffer), error);
                file->received += len;
                file->write(buffer.data(), len);

                uint32_t total = htonl(file->received);
                boost::asio::write(socket, boost::asio::buffer(&total, sizeof(total)));

                float percent = ((float)file->received / (float)file->size) * 100.0f;

                if (percent - old_percent > 5.0f)
                {
                    std::cout << "File '" << file->filename << "': "
                              << std::setprecision(4) << percent << "%"
                              << " (" << file->received << "/" << file->size << ")" << std::endl;

                    old_percent = percent;
                }

                if (error == boost::asio::error::eof || file->received == file->size)
                    break; // Connection closed cleanly by peer.
                else if (error)
                {
                    io_service.stop();
                    file->close();
                    throw boost::system::system_error(error); // Some other error.
                }

            };

            io_service.stop();
            file->close();

            std::cout << "Downloaded file!" << std::endl;
        }

    private:
        std::shared_ptr<DCCFile> file;

};

class DCCBot
{
    public:
        DCCBot(const std::string &host, const std::string &port, const std::string &nick, bool use_ssl)
            : connection(host, port, ([this](const std::string &msg) { this->read_handler(msg); }), use_ssl),
            threadpool(5)
        {
            connection.write((boost::format("NICK %s") % nick).str());
            connection.write((boost::format("USER %s * * %s") % nick % nick).str());
        } 

        void read_handler(const std::string &message)
        {
            IRCMessage msg(message);

            if (msg.type.compare("PRIVMSG") != 0)
                std::cout << "Received: '" << message << "' (CTCP: " << msg.ctcp << ")" << std::endl;

            if (msg.raw.substr(0, 4) == "PING")
            {
                connection.write("PONG " + msg.raw.substr(5));
                return;
            }

            if (msg.type == "001")
                on_connected();

            if (msg.type == "366")
                on_joined();

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

                    boost::asio::ip::address addr;
                    if (ip.find(':') != std::string::npos)
                    {
                        boost::asio::ip::address_v6 tmp(boost::asio::ip::address_v6::from_string(ip));
                        addr = boost::asio::ip::address(tmp);
                    }
                    else
                    {
                        unsigned long int_ip = std::stol(ip);
                        boost::asio::ip::address_v4 tmp(int_ip);
                        addr = boost::asio::ip::address(tmp);
                    }

                    std::cout << "DCC SEND request, offering file " << filename << " on ip " << addr.to_string() << ":" << port << std::endl;

                    std::lock_guard<std::mutex> lock(files_lock);
                    std::shared_ptr<DCCFile> file(std::make_shared<DCCFile>(addr.to_string(), port, filename, std::strtoumax(size.c_str(), nullptr, 10)));
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
            connection.write("JOIN #mg-chat");
            connection.write("JOIN #moviegods");
        }

        void on_joined()
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(8000));
            connection.write("PRIVMSG [MG]-HD|EU|S|Holly :XDCC SEND 209");
        }

    private:
        IRCConnection connection;
        std::vector<std::shared_ptr<DCCFile>> files;
        std::mutex files_lock;
        Threadpool threadpool;
};

int main(int argc, char* argv[])
{

    DCCBot bot("irc.abjects.net", "9999", "bahs3b34d", true);
    //DCCBot bot("irc.abjects.net", "6667", "bahs3b34d", false);
    //DCCBot bot("localhost", "6667", "test123");
    bot.run();

    std::cout << "Quitting ..." << std::endl;
    return 0;
}
