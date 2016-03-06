#include <iostream>
#include <chrono>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>

#include "api.h"

void setup_logging()
{
    // Code based on https://gist.github.com/xiongjia/e23b9572d3fc3d677e3d
    boost::log::add_common_attributes();
    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::debug
    );

    auto fmtTimeStamp = boost::log::expressions::
        format_date_time<boost::posix_time::ptime>("TimeStamp", "%d-%m-%Y %H:%M:%S.%f");
    auto fmtThreadId = boost::log::expressions::
        attr<boost::log::attributes::current_thread_id::value_type>("ThreadID");
    auto fmtSeverity = boost::log::expressions::
        attr<boost::log::trivial::severity_level>("Severity");
    boost::log::formatter logFmt =
        boost::log::expressions::format("[%1%] (%2%) [%3%] %4%")
        % fmtTimeStamp % fmtThreadId % fmtSeverity
        % boost::log::expressions::smessage;

    auto consoleSink = boost::log::add_console_log(std::clog);
    consoleSink->set_formatter(logFmt);

    /* fs sink */
    /*auto fsSink = boost::log::add_file_log(
        boost::log::keywords::file_name = "test_%Y-%m-%d_%H-%M-%S.%N.log",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::min_free_space = 30 * 1024 * 1024,
        boost::log::keywords::open_mode = std::ios_base::app);
    fsSink->set_formatter(logFmt);
    fsSink->locked_backend()->auto_flush(true);
    */
}

void signal_handler(int signal)
{
    BOOST_LOG_TRIVIAL(warning) << "Received SIGINT, shutting down ...";
    xdccd::API::quit = true;
}

int main(int argc, char* argv[])
{
    std::srand(std::time(0));

    setup_logging();
    BOOST_LOG_TRIVIAL(debug) << "Starting up xdccd!";

    std::signal(SIGINT, signal_handler);
    xdccd::API api;
    //api.get_bot_manager().launch_bot("irc.abjects.net", "6667", "asjd72z", { "#moviegods", "#mg-chat" }, false);
    api.get_bot_manager().launch_bot("irc.abjects.net", "6667", "nnkh", { "#moviegods", "#mg-chat" }, false);
    api.run();

    BOOST_LOG_TRIVIAL(info) << "Quitting ...";

    return 0;
}
