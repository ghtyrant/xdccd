#include "filetarget.h"

xdccd::FileTarget::FileTarget(file_id_t id, const std::string &filename, file_size_t size, const boost::filesystem::path &base_path)
    : AbstractTarget(id, filename, size),
    path(base_path)
{
    path /= filename;
}

void xdccd::FileTarget::open()
{
    stream.open(path, std::ios::binary);
}

void xdccd::FileTarget::close()
{
    stream.close();
}

void xdccd::FileTarget::write(const char* data, std::streamsize len)
{
    stream.write(data, len);
    received += len;
}

int xdccd::FileTarget::read()
{
    return 0;
}
