#pragma once

#include <json/json.h>
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

        const Json::Value &get() const;
        const Json::Value &operator[](const std::string &key) const;

        bool load();

    private:
        static boost::filesystem::path find_config();
        boost::filesystem::path path;
        Json::Value config;
};

}
