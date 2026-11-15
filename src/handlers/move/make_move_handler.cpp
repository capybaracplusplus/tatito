#include <boost/io/ios_state.hpp>
#include <handlers/move/make_move_handler.h>
#include <utils/http_response.h>

void MakeMoveHandler::handle_request(boost::asio::io_context &, http_request, Relay relay) {
    HttpResponse response;
    response.set_status(200);
    response.set_header("Content-Type", "text/plain");
    response.set_header("Connection", "close");
    response.set_body("aboba");

    relay(std::move(response));
}
