#ifndef SESSION_H
#define SESSION_H

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include <routes/router.h>
#include <utils/http_response.h>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(
        boost::asio::io_context &io_context,
        tcp::socket socket,
        std::function<void(const std::string &)> on_close
    );

    void start();

    void set_uuid(const std::string &uuid);

    void notify(const std::string &message);

private:
    static constexpr std::size_t max_header_size = 16 * 1024;
    static constexpr std::size_t max_body_size = 1024 * 1024;

    enum class WriteAction {
        none,
        read_next_request,
        close_connection,
    };

    struct PendingWrite {
        std::string data;
        WriteAction action;
    };

    void read_request();

    void handle_request_body(http_request req, std::string body);

    void write_error_response(int status, std::string message);

    void write_response(HttpResponse response, WriteAction action = WriteAction::read_next_request);

    void enqueue_write(std::string data, WriteAction action);

    void write_next();

    void close();

private:
    tcp::socket socket_;
    boost::asio::io_context &io_context_;

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    boost::asio::streambuf buffer_;
    std::deque<PendingWrite> write_queue_;

    static router router_;
    std::string uuid_;
    std::function<void(const std::string &)> on_close_;
    bool closed_ = false;
};

#endif // SESSION_H
