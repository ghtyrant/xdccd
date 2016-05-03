#include "downloadmanager.h"

xdccd::DownloadManager::DownloadManager(const boost::filesystem::path &download_path)
    : last_file_id(0),
      threadpool(5),
      download_path(download_path)
{

}

void xdccd::DownloadManager::start_download(const std::string &host,
        const std::string &port,
        const std::string &filename,
        xdccd::file_size_t size,
        bool active,
        bool buffer)
{
    std::lock_guard<std::mutex> lock(transfers_lock);

    AbstractTargetPtr target;

    if (buffer)
        target = std::make_shared<BufferTarget>(last_file_id++, host, port, filename, size);
    else
        target = std::make_shared<FileTarget>(last_file_id++, host, port, filename, size, download_path);

    DCCReceiveTaskPtr task = std::make_shared<DCCReceiveTask>(target, active, std::bind(&DownloadManager::on_file_finished, this, std::placeholders::_1));
    transfers[target->id] = task;

    threadpool.run_task(task);
}
