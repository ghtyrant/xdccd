#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <boost/log/trivial.hpp>

#include "task.h"

namespace xdccd
{

class ThreadManager
{
    public:
        ThreadManager()
            : stop(false)
        {
        }

        ~ThreadManager()
        {
            BOOST_LOG_TRIVIAL(info) << "ThreadManager::~ThreadManager";
            stop = true;

            join_all();
        }

        void join_all()
        {
            try
            {
                for (auto &t : threads)
                {
                    t.second.join();
                }
            }
            catch (...) {}
        }

        template <typename T>
        void run(T task)
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::thread t = std::thread(std::bind(&ThreadManager::wrap_task,
                        this, task));

            // Detach threads so they can erase themselves from the map without std::terminate being called
            t.detach();
            threads[t.get_id()] = std::move(t);
        }

        void run_task(TaskPtr task)
        {
            run([task](){ task->run(); });
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
            threads.erase(std::this_thread::get_id());
        }

        std::unordered_map<std::thread::id, std::thread> threads;
        std::mutex mutex;
        bool stop;
};
}
