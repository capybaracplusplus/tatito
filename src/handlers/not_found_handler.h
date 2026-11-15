#ifndef NOTFOUNDHANDLER_H
#define NOTFOUNDHANDLER_H

#include <handlers/base_handler.h>

class NotFoundHandler : public Handler {
public:
    void
    handle_request(boost::asio::io_context &io_context, http_request req, Relay relay) override;
};

#endif // NOTFOUNDHANDLER_H
