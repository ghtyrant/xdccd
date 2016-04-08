#pragma once

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

namespace xdccd
{

class Config
{
    public:
        Config();
        Config(const boost::filesystem::path &path);
        Config(const std::string &path);

    private:
        boost::filesystem::path &find_config() const;

        boost::filesystem::path path;

};

}
