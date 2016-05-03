#include "downloadmanager.h"
#include "filetarget.h"
#include "buffertarget.h"

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
        bool stream)
{
    AbstractTargetPtr target;

    if (stream)
        target = std::make_shared<BufferTarget>(last_file_id++, filename, size);
    else
        target = std::make_shared<FileTarget>(last_file_id++, filename, size, download_path);

    DCCReceiveTaskPtr task = std::make_shared<DCCReceiveTask>(host, port, target, active, std::bind(&DownloadManager::on_file_finished, this, std::placeholders::_1));

    std::lock_guard<std::mutex> lock(transfers_lock);
    BOOST_LOG_TRIVIAL(info) << "Created new AbstractTarget with id " << target->id << ", task: " << task.use_count() << ".";
    transfers[target->id] = task;
    BOOST_LOG_TRIVIAL(info) << "Created new AbstractTarget with id " << target->id << ", task: " << task.use_count() << ".";

    threadpool.run_task(task);
    BOOST_LOG_TRIVIAL(info) << "Created new AbstractTarget with id " << target->id << ", task: " << task.use_count() << ".";
}

void xdccd::DownloadManager::on_file_finished(file_id_t file_id)
{
    DCCReceiveTaskPtr task = transfers[file_id];
    AbstractTargetPtr target = task->get_target();
}

const std::vector<xdccd::AbstractTargetPtr> &xdccd::DownloadManager::get_finished_files() const
{
    return finished_files;
}

const std::map<xdccd::file_id_t, xdccd::DCCReceiveTaskPtr> &xdccd::DownloadManager::get_transfers() const
{
    return transfers;
}
