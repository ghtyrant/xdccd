#pragma once

#include <boost/asio.hpp>

#include "dccfile.h"

namespace xdccd
{

class DCCReceiveTask
{
    public:
        DCCReceiveTask(DCCFilePtr file, bool active = true);
        void operator()();

    private:
        bool connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);
        bool listen(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);

        boost::system::error_code download(boost::asio::ip::tcp::socket &socket);

        DCCFilePtr file;
        bool active;
};

}
