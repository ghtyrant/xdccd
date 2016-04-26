#pragma once

namespace xdccd
{

class Task
{
    public:
        Task() : quit(false) {};
        virtual void run() = 0;
        virtual void stop() { quit = true; };

    protected:
        bool quit;
};

typedef std::shared_ptr<Task> TaskPtr;

}
