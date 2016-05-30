#include <boost/filesystem.hpp>
#include <cstring>
#include <fstream>

#include "config.h"
#include "logging.h"

xdccd::Config::Config()
    : Config(xdccd::Config::find_config())
{
}

xdccd::Config::Config(const std::string &path)
    : Config(boost::filesystem::path(path))
{
    if (path.empty())
        this->path = xdccd::Config::find_config();
}

xdccd::Config::Config(const boost::filesystem::path &path)
    : path(path)
{
}

boost::filesystem::path xdccd::Config::find_config()
{
    boost::filesystem::path p("/etc/xdccd.conf");

    if (boost::filesystem::exists(p))
        return p;

    p = "xdccd.conf";

    if (boost::filesystem::exists(p))
        return p;

    return boost::filesystem::path();
}

bool xdccd::Config::load()
{
    if (path.empty())
    {
        BOOST_LOG_TRIVIAL(warning) << "No configuration file found!";
        return false;
    }

    Json::Reader reader;
    std::ifstream stream(path.string());

    if (!stream.is_open() || !stream.good())
    {
        BOOST_LOG_TRIVIAL(error) << "Error opening configuration file '" << path.string() << "': "
			<< std::strerror(errno) << " (error " << errno << ")";

        return false;
    }

    if (!reader.parse(stream, config, false))
    {
        BOOST_LOG_TRIVIAL(error) << "Error parsing configuration file '" << path.string() << "': "
            << reader.getFormattedErrorMessages();
        return false;
    }

    stream.close();

    return true;
}

const Json::Value &xdccd::Config::get() const
{
    return config;
}

const Json::Value &xdccd::Config::operator[](const std::string &key) const
{
    return config[key];
}
