#include "ircmessage.h"

xdccd::IRCMessage::IRCMessage(const std::string &message)
    : ctcp(false), raw(message)
{
    std::string msg = message;

    if (msg.empty())
        return;

    std::size_t space_pos;

    // Check for a prefix
    if (msg[0] == ':')
    {
        std::size_t space_pos = msg.find(' ');
        prefix = msg.substr(1, space_pos - 1);
        msg.erase(0, space_pos + 1);
    }

    // Parse command
    space_pos = msg.find(' ');
    command = msg.substr(0, space_pos);
    msg.erase(0, space_pos + 1);

    // In case there's a trailing parameter, store it for later
    std::size_t trailing_pos = msg.find(" :");
    std::string trailing;
    if (trailing_pos != std::string::npos)
    {
        trailing = msg.substr(trailing_pos + 2);
        msg.erase(trailing_pos, msg.length());
    }

    // Split all other parameters by space
    boost::algorithm::split(params, msg, boost::algorithm::is_space(), boost::algorithm::token_compress_on); 

    // If we found a trailing param earlier, add it now
    if (!trailing.empty())
        params.push_back(trailing);

    // Parse CTCP messages
    if (command == "PRIVMSG" && params[1][0] == 0x01 && params[1][params[1].length()-1] == 0x01)
    {
        ctcp = true;
        params[1] = params[1].substr(1, params[1].length() - 2);

        std::string ctcp_msg = params[1];
        space_pos = ctcp_msg.find(' ');
        ctcp_command = ctcp_msg.substr(0, space_pos);
        ctcp_msg.erase(0, space_pos + 1);

        space_pos = ctcp_msg.find(' ');
        while (space_pos != std::string::npos)
        {
            std::string param = ctcp_msg.substr(0, space_pos);
            if (ctcp_msg[0] == '"')
            {
                space_pos = ctcp_msg.find('"', 1);
                param = ctcp_msg.substr(1, space_pos - 1);
                space_pos++;
            }

            ctcp_msg.erase(0, space_pos + 1);
            ctcp_params.push_back(param);
            space_pos = ctcp_msg.find(" ");
        } 

        if (!ctcp_msg.empty())
            ctcp_params.push_back(ctcp_msg);
    }
}
