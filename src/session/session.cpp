#include <session/session.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

router Session::router_;

Session::Session(boost::asio::io_context &io_context, tcp::socket socket)
    : socket_(std::move(socket)), io_context_(io_context), strand_(boost::asio::make_strand(io_context)) {
}

void Session::start() {
    auto self = shared_from_this();
    boost::asio::dispatch(
        strand_, [self]() {
            self->read_request();
        });
}

void Session::set_uuid(const std::string &uuid) { uuid_ = uuid; }

void Session::notify(const std::string &message) {
    auto self = shared_from_this();
    std::string message_data = message;

    boost::asio::dispatch(
        strand_, [self,message_data = std::move(message_data)]() mutable {
            self->enqueue_write(std::move(message_data), false);
        }
    );
}

void Session::write_response(HttpResponse response) {
    auto self = shared_from_this();
    std::string response_data = response.to_string();


    boost::asio::dispatch(
        strand_, [self,response_data = std::move(response_data)]() mutable {
            self->enqueue_write(std::move(response_data), true);
        }
    );
}

http_request process_headers(const std::string &raw_headers) {
    http_request parsed_request;
    std::istringstream stream(raw_headers);
    std::string line;

    if (std::getline(stream, line)) {
        std::istringstream line_stream(line);
        line_stream >> parsed_request.method >> parsed_request.path >>
                parsed_request.version;
    }

    while (std::getline(stream, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            while (!value.empty() && value.front() == ' ')
                value.erase(value.begin());
            parsed_request.headers[key] = value;
        }
    }

    return parsed_request;
}

void Session::read_request() {
    auto self = shared_from_this();

    boost::asio::async_read_until(
        socket_, buffer_, "\r\n\r\n",
        boost::asio::bind_executor(strand_,
                                   [this, self](boost::system::error_code ec, std::size_t header_length) {
                                       if (ec) {
                                           if (ec == boost::asio::error::eof) {
                                               BOOST_LOG_TRIVIAL(info) << "Connection closed by client (EOF).";
                                           } else {
                                               BOOST_LOG_TRIVIAL(error) << "Header read error: " << ec.message();
                                           }
                                           return;
                                       }

                                       std::istream stream(&buffer_);
                                       std::string headers_str(header_length, '\0');
                                       stream.read(&headers_str[0], header_length);

                                       http_request req = process_headers(headers_str);
                                       req.uuid = uuid_;

                                       std::size_t content_length = 0;
                                       if (req.headers.count("Content-Length")) {
                                           content_length = std::stoul(req.headers["Content-Length"]);
                                       }

                                       std::string remaining((std::istreambuf_iterator<char>(stream)), {});
                                       std::size_t already_read = remaining.size();

                                       if (already_read >= content_length) {
                                           std::string body_str = remaining.substr(0, content_length);
                                           std::string trimmed = body_str;
                                           trimmed.erase(
                                               std::remove_if(trimmed.begin(), trimmed.end(), ::isspace),
                                               trimmed.end());

                                           if (!trimmed.empty()) {
                                               try {
                                                   req.body = json::parse(body_str);
                                               } catch (const json::parse_error &e) {
                                                   BOOST_LOG_TRIVIAL(error)
                  << "JSON parse error (partial body): " << e.what();
                                                   req.body = json::object();
                                               }
                                           } else {
                                               req.body = json::object();
                                           }

                                           router_.route_request(
                                               io_context_, std::move(req),
                                               [self](HttpResponse response) {
                                                   self->write_response(std::move(response));
                                               });
                                       } else {
                                           boost::asio::async_read(
                                               socket_, buffer_,
                                               boost::asio::transfer_exactly(content_length - already_read),
                                               boost::asio::bind_executor(
                                                   strand_,
                                                   [this, self, req = std::move(req),
                                                       remaining = std::move(remaining)](boost::system::error_code ec,
                                               std::size_t) mutable {
                                                       if (ec) {
                                                           BOOST_LOG_TRIVIAL(error)
                      << "Body read error: " << ec.message();
                                                           return;
                                                       }

                                                       std::istream body_stream(&buffer_);
                                                       std::string extra((std::istreambuf_iterator<char>(body_stream)),
                                                                         {});
                                                       std::string full_body = remaining + extra;

                                                       std::string trimmed = full_body;
                                                       trimmed.erase(
                                                           std::remove_if(trimmed.begin(), trimmed.end(), ::isspace),
                                                           trimmed.end());

                                                       if (!trimmed.empty()) {
                                                           try {
                                                               req.body = json::parse(full_body);
                                                           } catch (const json::parse_error &e) {
                                                               BOOST_LOG_TRIVIAL(error)
                        << "JSON parse error (full body): " << e.what();
                                                               req.body = json::object();
                                                           }
                                                       } else {
                                                           req.body = json::object();
                                                       }

                                                       router_.route_request(
                                                           io_context_, std::move(req),
                                                           [self](HttpResponse response) {
                                                               self->write_response(std::move(response));
                                                           });
                                                   }));
                                       }
                                   }));
}

void Session::enqueue_write(std::string data, bool read_next_request) {
    const bool write_in_progress = !write_queue_.empty();

    write_queue_.emplace_back(PendingWrite{
        .data = std::move(data),
        .read_next_request = read_next_request,
    });

    if (!write_in_progress) {
        write_next();
    }
}

void Session::write_next() {
    auto self = shared_from_this();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_queue_.front().data),
        boost::asio::bind_executor(
            strand_,
            [self](
        boost::system::error_code ec,
        std::size_t /*length*/
    ) {
                if (ec) {
                    BOOST_LOG_TRIVIAL(error) << " write error: " << ec.message();
                    self->write_queue_.clear();
                    return;
                }

                const bool read_next_request = self->write_queue_.front().read_next_request;
                self->write_queue_.pop_front();


                if (!self->write_queue_.empty()) {
                    self->write_next();
                }

                if (read_next_request) {
                    self->read_request();
                }
            }
        )
    );
}
