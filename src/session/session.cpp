#include <session/session.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

std::string trim_header_value(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
        value.erase(value.begin());
    }

    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')
    ) {
        value.pop_back();
    }

    return value;
}

bool valid_header_name(const std::string &name) {
    return !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char symbol) {
        return std::isalnum(symbol) || symbol == '-';
    });
}

bool valid_method(const std::string &method) {
    return !method.empty() && std::all_of(method.begin(), method.end(), [](unsigned char symbol) {
        return std::isupper(symbol);
    });
}

std::optional<http_request> parse_request_headers(const std::string &raw_headers) {
    http_request req;
    std::istringstream stream(raw_headers);
    std::string line;

    if (!std::getline(stream, line)) {
        return std::nullopt;
    }

    std::istringstream request_line(line);
    std::string extra;
    if (!(request_line >> req.method >> req.path >> req.version) || request_line >> extra) {
        return std::nullopt;
    }

    if (!valid_method(req.method) || req.version != "HTTP/1.1" || req.path.empty() ||
        req.path.front() != '/') {
        return std::nullopt;
    }

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            break;
        }

        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            return std::nullopt;
        }

        std::string name = line.substr(0, separator);
        if (!valid_header_name(name)) {
            return std::nullopt;
        }

        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char symbol) {
            return static_cast<char>(std::tolower(symbol));
        });

        const bool inserted =
            req.headers.emplace(std::move(name), trim_header_value(line.substr(separator + 1)))
                .second;
        if (!inserted) {
            return std::nullopt;
        }
    }

    if (!req.headers.contains("host")) {
        return std::nullopt;
    }

    return req;
}

std::optional<std::size_t> parse_content_length(const http_request &req) {
    const auto header = req.headers.find("content-length");
    if (header == req.headers.end()) {
        return 0;
    }

    const std::string &value = header->second;
    if (value.empty()) {
        return std::nullopt;
    }

    std::size_t content_length = 0;
    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(), content_length);

    if (error != std::errc{} || end != value.data() + value.size()) {
        return std::nullopt;
    }

    return content_length;
}

bool header_contains_token(
    const http_request &req,
    const std::string &header_name,
    const std::string &expected_token
) {
    const auto header = req.headers.find(header_name);
    if (header == req.headers.end()) {
        return false;
    }

    std::istringstream stream(header->second);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim_header_value(std::move(token));
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char symbol) {
            return static_cast<char>(std::tolower(symbol));
        });

        if (token == expected_token) {
            return true;
        }
    }

    return false;
}

} // namespace

router Session::router_;

Session::Session(
    boost::asio::io_context &io_context,
    tcp::socket socket,
    std::function<void(const std::string &)> on_close
)
    : socket_(std::move(socket)),
      io_context_(io_context),
      strand_(boost::asio::make_strand(io_context)),
      read_timer_(io_context),
      write_timer_(io_context),
      buffer_(max_header_size),
      on_close_(std::move(on_close)) {}

void Session::start() {
    auto self = shared_from_this();
    boost::asio::dispatch(strand_, [self]() { self->read_request(); });
}

void Session::set_uuid(const std::string &uuid) {
    uuid_ = uuid;
}

void Session::notify(const std::string &message) {
    auto self = shared_from_this();
    std::string message_data = message;

    boost::asio::dispatch(strand_, [self, message_data = std::move(message_data)]() mutable {
        self->enqueue_write(std::move(message_data), WriteAction::none);
    });
}

void Session::write_response(HttpResponse response, WriteAction action) {
    auto self = shared_from_this();
    std::string response_data = response.to_string();

    boost::asio::dispatch(
        strand_,
        [self, response_data = std::move(response_data), action]() mutable {
            self->enqueue_write(std::move(response_data), action);
        }
    );
}

void Session::handle_request_body(http_request req, std::string body) {
    if (closed_) {
        return;
    }

    const bool empty_body = std::all_of(body.begin(), body.end(), [](unsigned char symbol) {
        return std::isspace(symbol);
    });

    if (empty_body) {
        req.body = json::object();
    } else {
        try {
            req.body = json::parse(body);
        } catch (const json::parse_error &error) {
            BOOST_LOG_TRIVIAL(error) << "JSON parse error: " << error.what();
            write_error_response(400, "invalid JSON body");
            return;
        }
    }

    const bool request_close = header_contains_token(req, "connection", "close");

    auto self = shared_from_this();
    router_
        .route_request(io_context_, std::move(req), [self, request_close](HttpResponse response) {
            const bool close_connection = request_close || response.should_close();
            if (close_connection) {
                response.set_header("Connection", "close");
            }

            self->write_response(
                std::move(response),
                close_connection ? WriteAction::close_connection : WriteAction::read_next_request
            );
        });
}

void Session::write_error_response(int status, std::string message) {
    json body;
    body["error"] = std::move(message);

    HttpResponse response(status);
    response.set_header("Content-Type", "application/json");
    response.set_header("Connection", "close");
    response.set_body(body.dump());
    write_response(std::move(response), WriteAction::close_connection);
}

void Session::read_request() {
    if (closed_) {
        return;
    }

    auto self = shared_from_this();
    start_read_timeout();

    boost::asio::async_read_until(
        socket_,
        buffer_,
        "\r\n\r\n",
        boost::asio::bind_executor(
            strand_,
            [self](boost::system::error_code ec, std::size_t header_length) {
                self->cancel_read_timeout();

                if (self->closed_) {
                    return;
                }

                if (ec) {
                    if (ec == boost::asio::error::not_found ||
                        ec == boost::asio::error::no_buffer_space) {
                        self->write_error_response(431, "request headers are too large");
                        return;
                    }

                    if (ec == boost::asio::error::eof) {
                        BOOST_LOG_TRIVIAL(info) << "Connection closed by client (EOF).";
                    } else {
                        BOOST_LOG_TRIVIAL(error) << "Header read error: " << ec.message();
                    }

                    self->close();
                    return;
                }

                if (header_length > max_header_size) {
                    self->write_error_response(431, "request headers are too large");
                    return;
                }

                std::istream stream(&self->buffer_);
                std::string headers_str(header_length, '\0');
                stream.read(&headers_str[0], header_length);

                auto parsed_request = parse_request_headers(headers_str);
                if (!parsed_request.has_value()) {
                    self->write_error_response(400, "invalid HTTP request");
                    return;
                }

                http_request req = std::move(parsed_request.value());
                req.uuid = self->uuid_;

                if (req.headers.contains("transfer-encoding")) {
                    self->write_error_response(400, "Transfer-Encoding is not supported");
                    return;
                }

                const auto content_length = parse_content_length(req);
                if (!content_length.has_value()) {
                    self->write_error_response(400, "invalid Content-Length");
                    return;
                }

                if (content_length.value() > max_body_size) {
                    self->write_error_response(413, "request body is too large");
                    return;
                }

                auto body = std::make_shared<std::string>(content_length.value(), '\0');
                const std::size_t buffered_body_size =
                    std::min(content_length.value(), self->buffer_.size());

                if (buffered_body_size > 0) {
                    std::istream body_stream(&self->buffer_);
                    body_stream.read(body->data(), buffered_body_size);
                }

                if (buffered_body_size == content_length.value()) {
                    self->handle_request_body(std::move(req), std::move(*body));
                    return;
                }

                self->start_read_timeout();
                boost::asio::async_read(
                    self->socket_,
                    boost::asio::buffer(
                        body->data() + buffered_body_size,
                        content_length.value() - buffered_body_size
                    ),
                    boost::asio::bind_executor(
                        self->strand_,
                        [self,
                         req = std::move(req),
                         body](boost::system::error_code ec, std::size_t) mutable {
                            self->cancel_read_timeout();

                            if (self->closed_) {
                                return;
                            }

                            if (ec) {
                                BOOST_LOG_TRIVIAL(error) << "Body read error: " << ec.message();
                                self->close();
                                return;
                            }

                            self->handle_request_body(std::move(req), std::move(*body));
                        }
                    )
                );
            }
        )
    );
}

void Session::enqueue_write(std::string data, WriteAction action) {
    if (closed_) {
        return;
    }

    const bool write_in_progress = !write_queue_.empty();

    write_queue_.emplace_back(PendingWrite{
        .data = std::move(data),
        .action = action,
    });

    if (!write_in_progress) {
        write_next();
    }
}

void Session::write_next() {
    if (closed_ || write_queue_.empty()) {
        return;
    }

    auto self = shared_from_this();
    start_write_timeout();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_queue_.front().data),
        boost::asio::bind_executor(
            strand_,
            [self](
                boost::system::error_code ec,
                std::size_t /*length*/
            ) {
                self->cancel_write_timeout();

                if (self->closed_) {
                    return;
                }

                if (ec) {
                    BOOST_LOG_TRIVIAL(error) << " write error: " << ec.message();
                    self->close();
                    return;
                }

                const WriteAction action = self->write_queue_.front().action;
                self->write_queue_.pop_front();

                if (action == WriteAction::close_connection) {
                    self->close();
                    return;
                }

                if (!self->write_queue_.empty()) {
                    self->write_next();
                }

                if (action == WriteAction::read_next_request) {
                    self->read_request();
                }
            }
        )
    );
}

void Session::start_read_timeout() {
    auto self = shared_from_this();
    read_timer_.expires_after(read_timeout);
    read_timer_
        .async_wait(boost::asio::bind_executor(strand_, [self](boost::system::error_code ec) {
            if (ec == boost::asio::error::operation_aborted || self->closed_) {
                return;
            }

            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "Read timer error: " << ec.message();
            } else {
                BOOST_LOG_TRIVIAL(info) << "Session read timeout";
            }

            self->close();
        }));
}

void Session::cancel_read_timeout() {
    read_timer_.cancel();
}

void Session::start_write_timeout() {
    auto self = shared_from_this();
    write_timer_.expires_after(write_timeout);
    write_timer_
        .async_wait(boost::asio::bind_executor(strand_, [self](boost::system::error_code ec) {
            if (ec == boost::asio::error::operation_aborted || self->closed_) {
                return;
            }

            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "Write timer error: " << ec.message();
            } else {
                BOOST_LOG_TRIVIAL(info) << "Session write timeout";
            }

            self->close();
        }));
}

void Session::cancel_write_timeout() {
    write_timer_.cancel();
}

void Session::close() {
    if (closed_) {
        return;
    }

    closed_ = true;
    write_queue_.clear();
    cancel_read_timeout();
    cancel_write_timeout();

    boost::system::error_code ignored_error;
    socket_.cancel(ignored_error);
    socket_.shutdown(tcp::socket::shutdown_both, ignored_error);
    socket_.close(ignored_error);

    if (on_close_) {
        auto on_close = std::move(on_close_);
        on_close(uuid_);
    }
}
