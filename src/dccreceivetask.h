#pragma once

#include <boost/asio.hpp>

#include "dccfile.h"
#include "task.h"

namespace xdccd
{

enum ReceiveTaskState
{
    AWAITING_CONNECTION,
    CONNECTION_ERROR,
    AWAITING_RESPONSE,
    DOWNLOADING,
    FINISHED,
    CANCELLED,
    ERROR
};


class DCCReceiveTask : public Task, public std::enable_shared_from_this<DCCReceiveTask>
{
    public:
        DCCReceiveTask(DCCFilePtr file, bool active, std::function<void(std::shared_ptr<DCCReceiveTask>)> finished_handler);
        void run();
        ReceiveTaskState get_state() const;
        DCCFilePtr get_file() const;

    private:
        bool connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);
        bool listen(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);

        boost::system::error_code download(boost::asio::ip::tcp::socket &socket);

        DCCFilePtr file;
        bool active;
        std::function<void(std::shared_ptr<DCCReceiveTask>)> on_finished;
        ReceiveTaskState state;
};

typedef std::shared_ptr<DCCReceiveTask> DCCReceiveTaskPtr;

}
