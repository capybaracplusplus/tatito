#ifndef SESSION_H
#define SESSION_H

#include <deque>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include <routes/router.h>
#include <utils/http_response.h>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(boost::asio::io_context &io_context, tcp::socket socket);

    void start();

    void set_uuid(const std::string &uuid);

    void notify(const std::string &message);

private:
    struct PendingWrite {
        std::string data;
        bool read_next_request;
    };

    void read_request();

    void write_response(HttpResponse response);

    void enqueue_write(std::string data, bool read_next_request);

    void write_next();

private:
    tcp::socket socket_;
    boost::asio::io_context &io_context_;

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    boost::asio::streambuf buffer_;
    std::deque<PendingWrite> write_queue_;

    static router router_;
    std::string uuid_;
};

#endif // SESSION_H
