#include "filetarget.h"

xdccd::FileTarget::FileTarget(file_id_t id, const boost::filesystem::path &base_path, const std::string &filename, std::uintmax_t size)
    : AbstractTarget(id, filename, size, 0, false, base_path)
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
}

int xdccd::FileTarget::read()
{
    return 0;
}
