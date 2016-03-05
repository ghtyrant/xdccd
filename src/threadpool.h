#pragma once

#include <functional>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

namespace xdccd
{

class Threadpool
{
    public:
        Threadpool(std::size_t size)
            : work(io_service),
            available(size)
        {
            for (std::size_t i = 0; i < size; i++)
            {
               threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
            }
        }

        ~Threadpool()
        {
            io_service.stop();

            BOOST_LOG_TRIVIAL(warning) << "Threadpool::~Threadpool()";

            try
            {
                threads.join_all();
            }
            catch (...) {}
        }

        template <typename Task> void run_task(Task task)
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (available == 0) return;
            available--;

            io_service.post(boost::bind(&Threadpool::wrap_task, this,
                            std::function<void()>(task)));
        }

    private:
        void wrap_task(std::function<void()> task)
        {
            try
            {
                task();
            }
            catch (...) {}

            std::lock_guard<std::mutex> lock(mutex);
            available++;
        }

        boost::asio::io_service io_service;
        boost::asio::io_service::work work;
        boost::thread_group threads;
        std::size_t available;
        std::mutex mutex;
};

typedef std::shared_ptr<Threadpool> ThreadpoolPtr;

}
