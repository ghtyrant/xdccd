#pragma once

#include <boost/log/sources/severity_channel_logger.hpp>

namespace xdccd
{

namespace logging
{
    enum severity_level
    {
        debug,
        info,
        warning,
        error,
        critical
    };

}

typedef boost::log::sources::severity_channel_logger_mt<xdccd::logging::severity_level, std::string> logger_type_t;
void setup_logging();

}
