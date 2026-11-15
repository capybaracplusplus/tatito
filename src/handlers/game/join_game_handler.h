#ifndef JOIN_GAME_HANDLER_H
#define JOIN_GAME_HANDLER_H

#include <handlers/base_handler.h>

class JoinGameHandler : public Handler {
public:
    void
    handle_request(boost::asio::io_context &io_context, http_request req, Relay relay) override;
};

#endif // JOIN_GAME_HANDLER_H
