#pragma once

namespace xdccd
{


class BufferTarget : AbstractTarget
{
    public:
        BufferTarget(file_id_t id, const std::string &filename, file_size_t size);
        void open();
        void close();
        void write(const char* data, std::streamsize len);

    private:
        boost::filesystem::path path;
        boost::filesystem::ofstream stream;
};

typedef std::shared_ptr<FileTarget> FileTargetPtr;

}
