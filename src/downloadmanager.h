#pragma once

#include <mutex>
#include <boost/filesystem/path.hpp>

#include "filetarget.h"
#include "dccreceivetask.h"

namespace xdccd
{

typedef file_size_t std::streamoff;

class DownloadManager
{
    public:
        DownloadManager(const boost::filesystem::path &download_path);
        void start_download(const std::string &host, const std::string &port, const std::string &filename, file_size_t size, bool active, bool buffer);

    private:
        void on_file_finished(file_id_t task);

        file_id_t last_file_id;
        Threadpool threadpool;

        boost::filesystem::path download_path;
        std::vector<FileTargetPtr> finished_files;
        std::mutex finished_files_lock;

        std::map<file_id_t, DCCReceiveTaskPtr> transfers;
        std::mutex transfers_lock;
};

}
