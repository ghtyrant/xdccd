#pragma once

#include <mutex>
#include <boost/filesystem/path.hpp>

#include "abstracttarget.h"
#include "dccreceivetask.h"
#include "threadpool.h"

namespace xdccd
{

class DownloadManager
{
    public:
        DownloadManager(const boost::filesystem::path &download_path);
        void start_download(const std::string &host, const std::string &port, const std::string &filename, file_size_t size, bool active, bool stream);

        const std::vector<AbstractTargetPtr> &get_finished_files() const;
        const std::map<file_id_t, DCCReceiveTaskPtr> &get_transfers() const;

    private:
        void on_file_finished(file_id_t file_id);

        file_id_t last_file_id;
        Threadpool threadpool;

        boost::filesystem::path download_path;
        std::vector<AbstractTargetPtr> finished_files;
        std::mutex finished_files_lock;

        std::map<file_id_t, DCCReceiveTaskPtr> transfers;
        std::mutex transfers_lock;
};

}
