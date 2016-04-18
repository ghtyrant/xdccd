#pragma once
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

namespace xdccd
{

typedef std::size_t file_id_t;

enum FileState
{
    AWAITING_CONNECTION,
    AWAITING_RESPONSE,
    DOWNLOADING,
    FINISHED,
    CANCELLED,
    ERROR
};

class DCCFile
{
    public:
        DCCFile(file_id_t id, const std::string &bot, const std::string &ip, const std::string &port, const std::string &filename, std::uintmax_t size);
        void open();
        void close();
        void write(const char* data, std::streamsize len);

        file_id_t id;
        std::string bot;
        std::string ip;
        std::string port;
        std::string filename;
        std::uintmax_t size;
        std::uintmax_t received;
        std::size_t bytes_per_second;
        bool transfer_started;
        boost::filesystem::path path;
        boost::filesystem::ofstream stream;
        FileState state;
        bool passive;
};

typedef std::shared_ptr<DCCFile> DCCFilePtr;

}
