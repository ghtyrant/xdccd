#include "buffertarget.h"

xdccd::BufferTarget::BufferTarget(file_id_t id, const std::string &filename, file_size_t size)
    : AbstractTarget(id, filename, size)
{
}

void xdccd::BufferTarget::open()
{
}

void xdccd::BufferTarget::close()
{
}

void xdccd::BufferTarget::write(const char* data, std::streamsize len)
{
}

int xdccd::BufferTarget::read()
{
    return 0;
}
