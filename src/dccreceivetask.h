#pragma once

#include <boost/asio.hpp>

#include "dccfile.h"

namespace xdccd
{

class DCCReceiveTask
{
    public:
        DCCReceiveTask(DCCFilePtr file, bool active = true);
        void operator()(bool& stop);

    private:
        bool connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);
        bool listen(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);

        boost::system::error_code download(boost::asio::ip::tcp::socket &socket, bool& stop);

        DCCFilePtr file;
        bool active;
};

}
