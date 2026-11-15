#ifndef MAIN_HANDLER_H
#define MAIN_HANDLER_H

#include <handlers/base_handler.h>

class NotifyHandler : public Handler {
public:
    void
    handle_request(boost::asio::io_context &io_context, http_request req, Relay relay) override;
};

#endif // MAIN_HANDLER_H
