#pragma once

#include <boost/asio.hpp>

#include "dccfile.h"

namespace xdccd
{

class DCCReceiveTask
{
    public:
        DCCReceiveTask(DCCFilePtr file);
        bool connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);
        void operator()();

    private:
        DCCFilePtr file;

};

}
