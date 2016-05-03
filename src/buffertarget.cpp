#include "buffertarget.h"

xdccd::BufferTarget::BufferTarget(file_id_t id, const boost::filesystem::path &base_path, const std::string &filename, file_size_t size)
    : AbstractTarget(id, filename, size),
    path(base_path)
{
    path /= filename;
}

void xdccd::BufferTarget::open()
{
    stream.open(path, std::ios::binary);
}

void xdccd::BufferTarget::close()
{
    stream.close();
}

void xdccd::BufferTarget::write(const char* data, std::streamsize len)
{
    stream.write(data, len);
}

int xdccd::BufferTarget::read()
{
    return 0;
}
