#pragma once

#include "abstracttarget.h"

namespace xdccd
{

class FileTarget : public AbstractTarget
{
    public:
        FileTarget(file_id_t id, const std::string &filename, file_size_t size, const boost::filesystem::path &path);
        void open();
        void close();
        void write(const char* data, std::streamsize len);
        int read();

    private:
        boost::filesystem::path path;
        boost::filesystem::ofstream stream;
};

typedef std::shared_ptr<FileTarget> FileTargetPtr;

}
