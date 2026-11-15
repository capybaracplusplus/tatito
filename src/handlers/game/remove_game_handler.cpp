#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <handlers/game/remove_game_handler.h>
#include <nlohmann/json.hpp>
#include <services/matchmaking_service.h>
#include <utils/http_response.h>
namespace asio = boost::asio;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;

using json = nlohmann::json;

void RemoveGameHandler::handle_request(
    boost::asio::io_context &io_context,
    http_request req,
    Relay relay
) {
    HttpResponse response;
    response.set_header("Content-Type", "application/json");
    response.set_header("Connection", "keep-alive");

    co_spawn(
        io_context,
        MatchmakingService::remove_game(io_context, req.uuid),
        [response = std::move(response),
         relay = std::move(relay)](std::exception_ptr error) mutable {
            if (error) {
                try {
                    std::rethrow_exception(error);
                } catch (const std::exception &ex) {
                    response.set_status(400);
                    json body;
                    body["error"] = ex.what();
                    response.set_body(body.dump());
                }
            } else {
                response.set_status(200);
                response.set_body("{}");
            }

            relay(std::move(response));
        }
    );
}
