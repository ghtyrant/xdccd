#include "dccfile.h"

xdccd::DCCFile::DCCFile(file_id_t id, const std::string &bot, const std::string &ip, const std::string &port, const boost::filesystem::path &base_path, const std::string &filename, std::uintmax_t size)
    : id(id), bot(bot), ip(ip), port(port), filename(filename), size(size), received(0), bytes_per_second(0), transfer_started(false), path(base_path)
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
