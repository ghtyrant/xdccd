#pragma once
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

namespace xdccd
{

typedef std::size_t file_id_t;
typedef std::streamoff file_size_t;

class AbstractTarget
{
    public:
        AbstractTarget(file_id_t id, const std::string &filename, file_size_t size)
            : id(id), filename(filename), size(size), received(0)
        {}

        virtual ~AbstractTarget() {}

        virtual void open() = 0;
        virtual void close() = 0;
        virtual void write(const char* data, std::streamsize len) = 0;
        virtual int read() = 0;

        file_id_t id;
        std::string filename;
        file_size_t size;
        file_size_t received;
};

typedef std::shared_ptr<AbstractTarget> AbstractTargetPtr;

}
