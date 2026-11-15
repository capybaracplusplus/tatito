#include <handlers/notify_handler.h>
#include <nlohmann/json.hpp>
#include <services/notification_service.h>
#include <utils/http_request.h>
#include <utils/http_response.h>

using json = nlohmann::json;

void NotifyHandler::handle_request(
    boost::asio::io_context &io_context,
    http_request req,
    Relay relay
) {
    json body = json::array();

    while (true) {
        auto msg = NotificationService::read_notice(io_context, req.uuid);
        if (msg.empty())
            break;
        body.push_back(msg);
    }

    HttpResponse response;
    response.set_status(200);
    response.set_header("Content-Type", "application/json");
    response.set_header("Connection", "keep-alive");
    response.set_body(body.dump());

    relay(std::move(response));
}
