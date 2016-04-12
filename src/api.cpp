#include <iostream>
#include <restbed>
#include <functional>
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#include <json/json.h>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

#include "api.h"
#include "searchcache.h"

using namespace std::placeholders;
using namespace std::chrono_literals;

bool xdccd::API::quit = false;

xdccd::API::API()
    : manager(10)
{
}

xdccd::BotManager &xdccd::API::get_bot_manager()
{
    return manager;
}

void xdccd::API::static_file_handler(std::shared_ptr<restbed::Session> session)
{
    const auto request = session->get_request();
    const std::string filename = request->get_path_parameter("filename");
    std::string base_path = "ui/";

    if (boost::algorithm::starts_with(request->get_path(), "/js/"))
        base_path += "js/";
    if (boost::algorithm::starts_with(request->get_path(), "/css/"))
        base_path += "css/";
    if (boost::algorithm::starts_with(request->get_path(), "/img/"))
        base_path += "img/";

    BOOST_LOG_TRIVIAL(info) << "Serving static file '" << (base_path + filename) << "'";

    std::ifstream stream(base_path + filename, std::ifstream::in);

    if (stream.is_open())
    {
        const std::string body = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        std::string content_type = "text/html";
        if (boost::algorithm::ends_with(filename, ".js"))
            content_type = "application/javascript";
        else if (boost::algorithm::ends_with(filename, ".css"))
            content_type = "text/css";
        else if (boost::algorithm::ends_with(filename, ".gif"))
            content_type = "image/gif";

        const std::multimap<std::string, std::string> headers
        {
            {"Content-Type", content_type},
            {"Content-Length", std::to_string(body.length())}
        };

        session->close(restbed::OK, body, headers);
    }
    else
        session->close(restbed::NOT_FOUND);
}

void xdccd::API::status_handler(std::shared_ptr<restbed::Session> session)
{
    Json::Value root(Json::ValueType::arrayValue);

    for(auto bot : manager.get_bots())
    {
        Json::Value child;
        child["id"] = static_cast<Json::UInt64>(bot->get_id());
        child["botname"] = bot->get_nickname();
        child["connection_state"] = bot->get_connection_state();
        child["host"] = bot->get_host() + ":" + bot->get_port();
        child["announces"] = static_cast<Json::UInt64>(bot->get_announces().size());

        for (auto channel_name : bot->get_channels())
        {
            child["channels"].append(channel_name);
        }

        child["downloads"].resize(0);
        for (auto file : bot->get_files())
        {
            Json::Value file_child;
            file_child["id"] = static_cast<Json::UInt64>(file->id);
            file_child["filename"] = file->filename;
            file_child["filesize"] = static_cast<Json::UInt64>(file->size);
            file_child["received"] = static_cast<Json::UInt64>(file->received);
            file_child["state"] = file->state;
            float received_percent = ((float)file->received / (float)file->size) * 100.0f;
            file_child["received_percent"] = received_percent;
            file_child["passive"] = file->passive;

            child["downloads"].append(file_child);
        }

        root.append(child);
    }

    std::ostringstream oss;
    oss << root;

    std::string result = oss.str();

    session->close(restbed::OK, result, { { "Content-Length", std::to_string(result.size()) }, { "Content-Type", "application/json" } });
}

void xdccd::API::connect_handler(std::shared_ptr<restbed::Session> session)
{
    const auto request = session->get_request();

    int content_length = 0;
    request->get_header("Content-Length", content_length);

    session->fetch(content_length, [this](const std::shared_ptr<restbed::Session> session, const restbed::Bytes& body)
    {
        Json::Value root;
        Json::Reader reader;

        std::string data(body.begin(), body.end());
        if (!reader.parse(data.c_str(), root))
        {
            session->close(restbed::UNPROCESSABLE_ENTITY);
            return;
        }

        if (!root.isMember("server")
                || !root.isMember("nickname")
                || !root.isMember("channels"))
        {

            session->close(restbed::UNPROCESSABLE_ENTITY);
            return;
        }

        std::vector<std::string> channels;
        const Json::Value channel_list = root["channels"];

        for (Json::ArrayIndex i = 0; i < channel_list.size(); ++i)
            channels.push_back(channel_list[i].asString());

        manager.launch_bot(root["server"].asString(), "6667", root["nickname"].asString(), channels, false);

        session->close(restbed::OK);
    } );
}

void xdccd::API::disconnect_handler(std::shared_ptr<restbed::Session> session)
{
    const auto& request = session->get_request();
    const std::string id = request->get_path_parameter("id");
    xdccd::DCCBotPtr bot = manager.get_bot_by_id(static_cast<xdccd::bot_id_t>(std::stoi(id)));

    if (bot == nullptr)
    {
        session->close(restbed::NOT_FOUND);
        return;
    }

    manager.stop_bot(bot);

    session->close(restbed::OK);
}

void xdccd::API::remove_file_from_list_handler(std::shared_ptr<restbed::Session> session)
{
    const auto& request = session->get_request();
    const std::string bot_id = request->get_path_parameter("id");
    xdccd::DCCBotPtr bot = manager.get_bot_by_id(static_cast<xdccd::bot_id_t>(std::stoi(bot_id)));

    if(bot == nullptr)
    {
        session->close(restbed::NOT_FOUND);
        return;
    }

    const std::string file_id = request->get_path_parameter("file");
    bot->remove_file(static_cast<xdccd::file_id_t>(std::stoi(file_id)));

    session->close(restbed::OK);
}

void xdccd::API::request_file_handler(std::shared_ptr<restbed::Session> session)
{
    const auto request = session->get_request();

    const std::string id = request->get_path_parameter("id");
    xdccd::DCCBotPtr bot = manager.get_bot_by_id(static_cast<xdccd::bot_id_t>(std::stoi(id)));

    if (bot == nullptr)
    {
        session->close(restbed::NOT_FOUND);
        return;
    }

    int content_length = 0;
    request->get_header("Content-Length", content_length);

    session->fetch(content_length, [this, bot](const std::shared_ptr<restbed::Session> session, const restbed::Bytes& body)
    {
        Json::Value root;
        Json::Reader reader;

        std::string data(body.begin(), body.end());
        if (!reader.parse(data.c_str(), root))
        {
            session->close(restbed::UNPROCESSABLE_ENTITY);
            return;
        }

        if (!root.isMember("nick")
                || !root.isMember("slot"))
        {

            session->close(restbed::UNPROCESSABLE_ENTITY);
            return;
        }

        bot->request_file(root["nick"].asString(), root["slot"].asString());

        session->close(restbed::OK);
    } );
}

void xdccd::API::search_handler(std::shared_ptr<restbed::Session> session)
{
    const auto request = session->get_request();

    int content_length = 0;
    request->get_header("Content-Length", content_length);

    session->fetch(content_length, [this](const std::shared_ptr<restbed::Session> session, const restbed::Bytes& body)
    {
        // Parse request
        Json::Value search_request;
        Json::Reader reader;

        std::string data(body.begin(), body.end());

        if (!reader.parse(data.c_str(), search_request))
        {
            session->close(restbed::UNPROCESSABLE_ENTITY);
            return;
        }

        if (!search_request.isMember("query") || search_request["query"].asString().empty())
        {
            session->close(restbed::UNPROCESSABLE_ENTITY);
            return;
        }

        std::size_t start = 0;
        if (search_request.isMember("start"))
            start = search_request["start"].asUInt64();

        std::size_t limit = xdccd::search::RESULTS_PER_PAGE;
        if (search_request.isMember("limit"))
            limit = search_request["limit"].asUInt64();

        // Build response
        Json::Value root;

        xdccd::SearchResultPtr sr = search.search(manager, search_request["query"].asString(), start, limit);

        root["total_results"] = static_cast<Json::UInt64>(sr->total_results);
        root["start"] = static_cast<Json::UInt64>(sr->result_start);

        Json::Value result_list(Json::ValueType::arrayValue);
        for (auto it = sr->begin; it != sr->end; ++it)
        {
            Json::Value child;
            DCCAnnouncePtr announce = *it;

            child["bot_id"] = static_cast<Json::UInt64>(announce->bot_id);
            child["name"] = announce->filename;
            child["size"] = announce->size;
            child["download_count"] = announce->download_count;
            child["bot"] = announce->bot_name;
            child["slot"] = announce->slot;
            result_list.append(child);
        }

        root["results"] = result_list;

        std::ostringstream oss;
        oss << root;
        std::string result = oss.str();
        session->close(restbed::OK, result, { { "Content-Length", std::to_string(result.size()) }, { "Content-Type", "application/json" } });
    });
}

void xdccd::API::shutdown_handler(std::shared_ptr<restbed::Session> session)
{
    session->close(restbed::OK);
    stop();
}

void xdccd::API::stop()
{
    service.stop();
}

void xdccd::API::run()
{
    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(1984);
    //ONLY FOR TESTING PURPOSE
    settings->set_default_header("Access-Control-Allow-Origin", "*");
    settings->set_default_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    settings->set_default_header("Access-Control-Max-Age", "604800");
    settings->set_default_header("Access-Control-Allow-Headers", "Content-Type");
    settings->set_default_header("Connection", "close");

    // Static Files
    auto resource = std::make_shared<restbed::Resource>();
    resource->set_paths({ "/{filename: [a-z]*\\.html}", "/js/{filename: [a-z]*\\.js}", "/css/{filename: [a-z]*\\.css}", "/img/{filename: [a-z]*\\.gif}" });
    resource->set_method_handler("GET", std::bind(&API::static_file_handler, this, std::placeholders::_1));
    service.publish(resource);

    // Status
    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/status");
    resource->set_method_handler("GET", { { "Content-Type", "application/json" } }, std::bind(&API::status_handler, this, std::placeholders::_1));
    service.publish(resource);

    // Connect
    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/connect");
    resource->set_method_handler("POST", { { "Content-Type", "application/json" } }, std::bind(&API::connect_handler, this, std::placeholders::_1));
    resource->set_method_handler("OPTIONS", [](std::shared_ptr<restbed::Session> session) { session->close(restbed::OK, ""); } );
    service.publish(resource);

    // Disconnect
    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/disconnect/{id: [0-9]+}");
    resource->set_method_handler("GET", { { "Content-Type", "application/json" } }, std::bind(&API::disconnect_handler, this, std::placeholders::_1));
    service.publish(resource);

    // Search
    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/search");
    resource->set_method_handler("POST", { { "Content-Type", "application/json" } }, std::bind(&API::search_handler, this, std::placeholders::_1));
    resource->set_method_handler("OPTIONS", [](std::shared_ptr<restbed::Session> session) { session->close(restbed::OK, ""); } );
    service.publish(resource);

    // Request File
    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/bot/{id: [0-9]+}/request/");
    resource->set_method_handler("POST", { { "Content-Type", "application/json" } }, std::bind(&API::request_file_handler, this, std::placeholders::_1));
    resource->set_method_handler("OPTIONS", [](std::shared_ptr<restbed::Session> session) { session->close(restbed::OK, ""); } );
    service.publish(resource);

    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/bot/{id: [0-9]+}/delete/{file: [0-9]+}");
    resource->set_method_handler("POST", { { "Content-Type", "application/json"} }, std::bind(&API::remove_file_from_list_handler, this, std::placeholders::_1));
    resource->set_method_handler("OPTIONS", [](std::shared_ptr<restbed::Session> session) { session->close(restbed::OK, ""); } );
    service.publish(resource);

    // Shutdown
    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/shutdown");
    resource->set_method_handler("GET", std::bind(&API::shutdown_handler, this, std::placeholders::_1));
    service.publish(resource);

    BOOST_LOG_TRIVIAL(info) << "Starting up REST API ...";

    // Start scheduled task to check if we should stop
    service.schedule([this](){ if (xdccd::API::quit) { service.stop(); } }, 200ms);
    service.start(settings);
}
