#include <handlers/handlers.h>
#include <memory>
#include <routes/router.h>
#include <routes/routes.h>
#include <vector>

router::router() { register_all_routes(); }

void router::route_request(
    boost::asio::io_context &io_context,
    http_request req,
    Handler::Relay relay
) {
    auto handler = routes_.get_handler(req.path, req.method);
    if (handler) {
        handler->handle_request(io_context, std::move(req), std::move(relay));
    } else {
        NotFoundHandler().handle_request(io_context, std::move(req), std::move(relay));
    }
}

void router::register_all_routes() {
    std::vector<std::shared_ptr<i_route_registrar> > route_modules = {
        std::make_shared<NotifyRoutes>(),
        std::make_shared<GameRoutes>(),
        std::make_shared<MoveRoutes>(),
    };

    for (const auto &module: route_modules) {
        module->register_routes(routes_);
    }
}
