#include "dccfile.h"

xdccd::DCCFile::DCCFile(const std::string &ip, const std::string &port, const std::string &filename, std::uintmax_t size)
    : ip(ip), port(port), filename(filename), size(size), received(0), transfer_started(false), path("downloads")
{
    path /= filename;
}

void xdccd::DCCFile::open()
{ 
    stream.open(path, std::ios::binary);
}

void xdccd::DCCFile::close()
{
    stream.close();
}

void xdccd::DCCFile::write(const char* data, std::streamsize len)
{
  stream.write(data, len);
}
