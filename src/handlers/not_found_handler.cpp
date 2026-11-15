#include <handlers/not_found_handler.h>

void NotFoundHandler::handle_request(boost::asio::io_context &, http_request req, Relay relay) {
    auto response = HttpResponse{404};
    response.set_header("Content-Type", "text/plain");
    response.set_body("404 Not Found: The requested path " + req.path + " does not exist.");

    relay(std::move(response));
}
