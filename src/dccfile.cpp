#include "dccfile.h"

xdccd::DCCFile::DCCFile(file_id_t id, const std::string &bot, const std::string &ip, const std::string &port, const std::string &filename, std::uintmax_t size)
    : id(id), bot(bot), ip(ip), port(port), filename(filename), size(size), received(0), transfer_started(false), path("downloads"), state(xdccd::FileState::AWAITING_CONNECTION)
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
