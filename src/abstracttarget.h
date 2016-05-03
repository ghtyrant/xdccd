#pragma once
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

namespace xdccd
{
    typedef std::size_t file_id_t;
    typedef file_size_t std::streamoff;

abstract class AbstractTarget
{
    public:
        AbstractTarget(file_id_t id, const boost::filesystem::path &base_path, const std::string &filename, std::uintmax_t size);
        virtual void open();
        virtual void close();
        virtual void write(const char* data, std::streamsize len);
        virtual int read();

        file_id_t id;
        std::string filename;
        std::uintmax_t size;
        std::uintmax_t received;
        bool transfer_started;
        boost::filesystem::path path;
        boost::filesystem::ofstream stream;
        bool passive;
};

typedef std::shared_ptr<DCCFile> DCCFilePtr;

}
