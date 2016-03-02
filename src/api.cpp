#include <iostream>
#include <restbed>
#include <functional>
#include <json/json.h>

#include "api.h"

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
        child["serverName"] = bot->get_host() + ":" + bot->get_port();

        for (auto channel_name : bot->get_channels())
        {
            child["channels"].append(channel_name);
        }

        for (auto file : bot->get_files())
        {
            Json::Value file_child;
            file_child["id"] = static_cast<Json::UInt64>(file->id);
            file_child["filename"] = file->filename;
            file_child["filesize"] = static_cast<Json::UInt64>(file->size);
            float received_percent = ((float)file->received / (float)file->size) * 100.0f;
            file_child["received"] = received_percent;

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

    auto resource = std::make_shared<restbed::Resource>();
    resource->set_path("/status");
    resource->set_method_handler("GET", { { "Content-Type", "application/json" } }, std::bind(&API::status_handler, this, std::placeholders::_1));
    service.publish(resource);

    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/connect");
    resource->set_method_handler("POST", { { "Content-Type", "application/json" } }, std::bind(&API::connect_handler, this, std::placeholders::_1));
    resource->set_method_handler("OPTIONS", [](std::shared_ptr<restbed::Session> session) { session->close(restbed::OK, ""); } );
    service.publish(resource);

    resource = std::make_shared<restbed::Resource>();
    resource->set_path("/disconnect/{id: [0-9]+}");
    resource->set_method_handler("GET", { { "Content-Type", "application/json" } }, std::bind(&API::disconnect_handler, this, std::placeholders::_1));
    service.publish(resource);

    service.start(settings);
}
