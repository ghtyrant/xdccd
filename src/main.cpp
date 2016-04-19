#include <iostream>
#include <chrono>
#include <vector>

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

    // Start API
    xdccd::API api(config);

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

            api.get_bot_manager().launch_bot(host, port, bot_name, channels, ssl);
        }
    }

    api.run();

    BOOST_LOG_TRIVIAL(info) << "Quitting ...";

    return 0;
}
