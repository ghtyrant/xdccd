#pragma once

#include <functional>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

#include "task.h"

namespace xdccd
{

class Threadpool
{
    public:
        Threadpool(std::size_t size)
            : work(io_service),
              available(size),
              stop(false)
        {
            for (std::size_t i = 0; i < size; i++)
            {
               threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
            }
        }

        ~Threadpool()
        {
            BOOST_LOG_TRIVIAL(info) << "Threadpool::~Threadpool";
            io_service.stop();
            stop = true;

            try
            {
                threads.join_all();
            }
            catch (...) {}
        }

        template <typename T>
        void run(T task)
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (available == 0)
                return;

            available--;

            io_service.post(boost::bind(&Threadpool::wrap_task, this,
                            task));
        }

        void run_task(TaskPtr task)
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (available == 0)
                return;

            available--;

            io_service.post(boost::bind(&Threadpool::wrap_task, this,
                            [&task](){ task->run(); }));
        }

    private:
        void wrap_task(std::function<void()> func)
        {
            try
            {
                func();
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
        bool stop;
};

typedef std::shared_ptr<Threadpool> ThreadpoolPtr;

}
