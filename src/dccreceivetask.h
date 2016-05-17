#pragma once

#include <boost/asio.hpp>

#include "abstracttarget.h"
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
        DCCReceiveTask(const std::string &host, const std::string &port, AbstractTargetPtr file, bool active, std::function<void(file_id_t)> finished_handler);
        ~DCCReceiveTask();
        void run();
        ReceiveTaskState get_state() const;
        AbstractTargetPtr get_target() const;
        std::size_t get_bps() const;
        bool is_active() const;

    private:
        bool connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);
        bool listen(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket);

        boost::system::error_code download(boost::asio::ip::tcp::socket &socket);

        std::string host;
        std::string port;
        AbstractTargetPtr target;
        bool active;
        std::function<void(file_id_t)> on_finished;
        ReceiveTaskState state;
        std::size_t bytes_per_second;
};

typedef std::shared_ptr<DCCReceiveTask> DCCReceiveTaskPtr;

}
