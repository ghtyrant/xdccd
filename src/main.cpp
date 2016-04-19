#include <iostream>
#include <chrono>
#include <vector>
#include <unistd.h>
#include <boost/filesystem.hpp>

#include "api.h"
#include "logging.h"
#include "config.h"

void signal_handler(int signal)
{
    BOOST_LOG_TRIVIAL(warning) << "Received SIGINT, shutting down ...";
    xdccd::API::quit = true;
}

int main(int argc, char* argv[])
{
    std::srand(std::time(0));

    xdccd::setup_logging();
    BOOST_LOG_TRIVIAL(debug) << "Starting up xdccd!";

    std::signal(SIGINT, signal_handler);

    xdccd::Config config;
    config.load();

    if (config["download_path"].isNull())
    {
        BOOST_LOG_TRIVIAL(error) << "Configuration error: 'download_path' is missing!";
        return 1;
    }

    boost::filesystem::path download_path(config["download_path"].asString());

    // Download path sanity checks
    if (!boost::filesystem::exists(download_path) || !boost::filesystem::is_directory(download_path))
    {
        BOOST_LOG_TRIVIAL(error) << "Configuration error: download_path not found or not a directory: " << download_path.string();
        return 1;
    }

    if (access(download_path.c_str(), W_OK | X_OK) != 0)
    {
        BOOST_LOG_TRIVIAL(error) << "Configuration error: Insufficient permissions to write to download_path: " << download_path.string();
        return 1;
    }

    // Start API
    std::string bind_address = config["api"].get("host", "127.0.0.1").asString();
    int port = config["api"].get("port", 1984).asInt();
    xdccd::API api(bind_address, port, download_path);

    // Start bots defined in config file
    Json::Value bots = config["bots"];
    if (!bots.isNull())
    {
        if (!bots.isObject())
        {
            BOOST_LOG_TRIVIAL(error) << "Configuration error: 'bots' has to be an object!";
            return 0;
        }

        for(Json::ValueIterator it = bots.begin() ; it != bots.end() ; ++it)
        {
            Json::Value bot = *it;
            std::string bot_name = it.key().asString();

            if (bot["host"].isNull() || bot["port"].isNull())
            {
                BOOST_LOG_TRIVIAL(error) << "Configuration error: bot '" << bot_name << "' is missing 'host' or 'port'!";
                return 1;
            }

            std::string host = bot["host"].asString();
            std::string port = bot["port"].asString();

            bool ssl = bot.get("ssl", false).asBool();
            std::vector<std::string> channels;
            if (!bot["channels"].isNull() && bot["channels"].isArray())
                for(auto channel : bot["channels"])
                    channels.push_back(channel.asString());

            api.get_bot_manager().launch_bot(host, port, bot_name, channels, ssl, download_path);
        }
    }

    api.run();

    BOOST_LOG_TRIVIAL(info) << "Quitting ...";

    return 0;
}
