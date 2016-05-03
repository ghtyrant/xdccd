#pragma once

#include "abstracttarget.h"

namespace xdccd
{

class BufferTarget : public AbstractTarget
{
    public:
        BufferTarget(file_id_t id, const std::string &filename, file_size_t size);
        void open();
        void close();
        void write(const char* data, std::streamsize len);
        int read();

    private:
};

typedef std::shared_ptr<BufferTarget> BufferTargetPtr;

}
