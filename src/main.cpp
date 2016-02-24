#include <iostream>
#include <chrono>

#include "threadpool.h"
#include "socket.h"
#include "ircmessage.h"
#include "ircconnection.h"
#include "dccbot.h"



int main(int argc, char* argv[])
{
    //DCCBot bot("irc.abjects.net", "9999", "bahs3b34d", true);
    //DCCBot bot("irc.abjects.net", "6667", "bahs3b34d", false);
    //DCCBot bot("localhost", "6667", "test123");
    xdccd::DCCBot bot("blitzforum.de", "6697", "test123", true);
    bot.run();

    std::cout << "Quitting ..." << std::endl;
    return 0;
}