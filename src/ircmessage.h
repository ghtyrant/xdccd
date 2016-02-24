#pragma once
#include <boost/algorithm/string.hpp>

namespace xdccd
{

class IRCMessage
{
    public:
        IRCMessage(const std::string &message);
        
        std::string prefix;
        std::string command;
        std::vector<std::string> params;
        bool ctcp;
        std::string ctcp_command;
        std::vector<std::string> ctcp_params;

        std::string raw;
};

}
