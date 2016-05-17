#include "downloadmanager.h"
#include "filetarget.h"
#include "buffertarget.h"

xdccd::DownloadManager::DownloadManager(ThreadManager &thread_man, const boost::filesystem::path &download_path)
    : last_file_id(0),
      thread_manager(thread_man),
      download_path(download_path)
{
}

void xdccd::DownloadManager::start_download(const std::string &host,
        const std::string &port,
        const std::string &filename,
        xdccd::file_size_t size,
        bool active,
        bool stream)
{
    AbstractTargetPtr target;

    if (stream)
        target = std::make_shared<BufferTarget>(last_file_id++, filename, size);
    else
        target = std::make_shared<FileTarget>(last_file_id++, filename, size, download_path);

    DCCReceiveTaskPtr task = std::make_shared<DCCReceiveTask>(host, port, target, active, std::bind(&DownloadManager::on_file_finished, this, std::placeholders::_1));

    std::lock_guard<std::mutex> lock(transfers_lock);
    transfers[target->id] = task;

    thread_manager.run_task(task);
}

void xdccd::DownloadManager::on_file_finished(file_id_t file_id)
{
    std::lock_guard<std::mutex> lock(transfers_lock);
    DCCReceiveTaskPtr task = transfers[file_id];
    AbstractTargetPtr target = task->get_target();
}

std::vector<xdccd::AbstractTargetPtr> xdccd::DownloadManager::get_finished_files()
{
    std::lock_guard<std::mutex> lock(finished_files_lock);
    return finished_files;
}

std::map<xdccd::file_id_t, xdccd::DCCReceiveTaskPtr> xdccd::DownloadManager::get_transfers()
{
    std::lock_guard<std::mutex> lock(transfers_lock);
    return transfers;
}
