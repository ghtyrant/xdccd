#include <iostream>
#include <chrono>

#include "api.h"
#include "logging.h"

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
    xdccd::API api;
    //api.get_bot_manager().launch_bot("irc.abjects.net", "6667", "nnkhsdfsdfz2", { "#moviegods", "#mg-chat" }, false);
    api.get_bot_manager().launch_bot("localhost", "6667", "nnkh", { "#moviegods", "#mg-chat" }, false);
    api.run();

    BOOST_LOG_TRIVIAL(info) << "Quitting ...";

    return 0;
}
