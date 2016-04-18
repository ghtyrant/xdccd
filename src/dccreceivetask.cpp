#include <iostream>
#include <iomanip>
#include <chrono>

#include "dccreceivetask.h"
#include "logging.h"

xdccd::DCCReceiveTask::DCCReceiveTask(std::shared_ptr<DCCFile> file, bool active)
    : file(file), active(active)
{}

bool xdccd::DCCReceiveTask::connect(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket)
{
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(file->ip, file->port);
    boost::system::error_code error = boost::asio::error::host_not_found;

    auto iter = resolver.resolve(query);
    decltype(iter) end;

    while (iter != end)
    {
        if (!error)
            break;

        socket.close();
        socket.connect(*iter++, error);

        if (error)
            std::cout << "Error connecting to " << file->ip << ": " << error.message() << std::endl;
    }

    if (error)
        return false;

    return true;
}

bool xdccd::DCCReceiveTask::listen(boost::asio::io_service &io_service, boost::asio::ip::tcp::socket &socket)
{
    boost::asio::ip::tcp::acceptor a(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 12345));

    for (;;)
    {
        a.accept(socket);
        return true;
    }

    return false;
}


void xdccd::DCCReceiveTask::operator()(bool &stop)
{
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket(io_service);

    if (active)
    {
        if (!connect(io_service, socket))
            return;
    }
    else
    {
        if (!listen(io_service, socket))
            return;
    }

    file->state = xdccd::FileState::DOWNLOADING;

    BOOST_LOG_TRIVIAL(info) << "Starting " << (active ? "active" : "passive") << " download of file '" << file->filename << "'";

    file->passive = !active;
    file->open();

    boost::system::error_code result = download(socket, stop);

    io_service.stop();
    file->close();

    if (result != boost::system::errc::success)
    {
        file->state = xdccd::FileState::ERROR;
        BOOST_LOG_TRIVIAL(info) << "Error downloading file '" << file->filename << "'";
        throw boost::system::system_error(result);
    }
    else
        file->state = xdccd::FileState::FINISHED;

    BOOST_LOG_TRIVIAL(info) << "Finished downloading file '" << file->filename << "'";
}

boost::system::error_code xdccd::DCCReceiveTask::download(boost::asio::ip::tcp::socket &socket, bool &stop)
{
    boost::system::error_code error;
    error.assign(boost::system::errc::success, boost::system::system_category());
    std::array<char, 8196> buffer;
    float old_percent = 0.0f;

    while (!stop)
    {
        auto start = std::chrono::system_clock::now();

        std::size_t len = socket.read_some(boost::asio::buffer(buffer), error);
        file->received += len;
        file->write(buffer.data(), len);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<float, std::milli> elapsed = end - start;

        file->bytes_per_second = (len / elapsed.count()) * 1000.0;

        // Send back how much we've downloaded in total
        // This has to be a 32bit integer, because DCC is old and sucks
        uint32_t total = htonl(file->received);
        boost::asio::write(socket, boost::asio::buffer(&total, sizeof(total)));

        float percent = ((float)file->received / (float)file->size) * 100.0f;

        if (percent - old_percent > 5.0f)
        {
            BOOST_LOG_TRIVIAL(info) << "File '" << file->filename << "': "
                      << std::setprecision(4) << percent << "%"
                      << " (" << file->received << "/" << file->size << ")";

            old_percent = percent;
        }

        if (error == boost::asio::error::eof || file->received == file->size)
            break; // Connection closed cleanly by peer.
        else if (error)
            break;
    }

    BOOST_LOG_TRIVIAL(info) << "Stopping dowload of file '" << file->filename << "'";

    return error;
}
