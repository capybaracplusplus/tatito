#ifndef CREATE_GAME_HANDLER_H
#define CREATE_GAME_HANDLER_H

#include <handlers/base_handler.h>

class CreateGameHandler : public Handler {
public:
    void handle_request(
        boost::asio::io_context &io_context,
        http_request req,
        Relay relay
    ) override;
};

#endif // CREATE_GAME_HANDLER_H
