#ifndef HANDLER_H
#define HANDLER_H

#include <boost/asio/io_context.hpp>
#include <functional>
#include <utils/http_request.h>
#include <utils/http_response.h>


class Handler {
public:
    using Relay = std::function<void(HttpResponse)>;

    virtual ~Handler() = default;

    virtual void handle_request(
        boost::asio::io_context &io_context,
        http_request req,
        Relay relay
    ) = 0;
};

#endif // HANDLER_H
