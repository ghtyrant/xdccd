#pragma once
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

namespace xdccd
{

typedef std::size_t file_id_t;
typedef file_size_t std::streamoff;

class AbstractTarget
{
    public:
        AbstractTarget(file_id_t id, const std::string &filename, file_size_t size);
        virtual void open();
        virtual void close();
        virtual void write(const char* data, std::streamsize len);
        virtual int read();

        file_id_t id;
        std::string filename;
        file_size_t size;
        file_size_t received;
        bool passive;
};

typedef std::shared_ptr<AbstractTarget> AbstractTargetPtr;

}
