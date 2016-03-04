#include <iostream>
#include <restbed>
#include <functional>
#include <json/json.h>

#include "api.h"
#include "searchcache.h"

using namespace std::placeholders;

xdccd::API::API()
    : manager(10)
{
}

xdccd::BotManager &xdccd::API::get_bot_manager()
{
    return manager;
}

void xdccd::API::status_handler(std::shared_ptr<restbed::Session> session)
{
    Json::Value root(Json::ValueType::arrayValue);

    for(auto bot : manager.get_bots())
    {
        Json::Value child;
        child["id"] = static_cast<Json::UInt64>(bot->get_id());
        child["botname"] = bot->get_nickname();
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
        std::cout << "FILE-REQUEST-BODY: " << data << std::endl;
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

        // Build response
        Json::Value root;
        
        xdccd::SearchResultPtr sr = xdccd::SearchCache::get_instance().search(manager, search_request["query"].asString(), start);
        std::cout << "Searching for '" << search_request["query"].asString() << "' yields " << sr->total_results << " results, starting with " << sr->result_start << std::endl;

        root["total_results"] = static_cast<Json::UInt64>(sr->total_results);
        root["start"] = static_cast<Json::UInt64>(sr->result_start);

        Json::Value result_list(Json::ValueType::arrayValue);
        for (auto announce : sr->announces)
        {
            Json::Value child;
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

    // Status
    auto resource = std::make_shared<restbed::Resource>();
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

    service.start(settings);
}
