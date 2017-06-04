#include "http_upstream_request.h"

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "http.h"
#include "timer.h"

namespace servx {

HttpUpstreamRequest::~HttpUpstreamRequest() {
    if (request_header_buf) {
        server->ret_body_buf(request_header_buf);
    }
    if (response_header_buf) {
        server->ret_body_buf(response_header_buf);
    }
    for (Buffer *buf : request_body_bufs) {
        server->ret_body_buf(buf);
    }
    for (Buffer *buf : response_body_bufs) {
        server->ret_body_buf(buf);
    }
}

void HttpUpstreamRequest::close(int rc) {
    // TODO: keep-alive
    finalize_handler(request, rc);
    conn->close();
}

int HttpUpstreamRequest::connect() {
    socket = std::unique_ptr<TcpConnectSocket>(
        new TcpConnectSocket(server->get_socket()));
    int rc = socket->connect();

    if (rc == SERVX_ERROR) {
        Logger::instance()->warn("connect error");
        return SERVX_ERROR;
    }

    Logger::instance()->debug("connect fd = %d", socket->get_fd());

    conn = ConnectionPool::instance()
        ->get_connection(socket->get_fd(), false);

    if (conn == nullptr || !add_connection(conn)) {
        Logger::instance()->warn("add connection error");
        conn->close();
        return SERVX_ERROR;
    }

    if (rc == SERVX_AGAIN) {
        Logger::instance()->debug("connect again");
        conn->get_read_event()->set_handler(empty_read_handler);
        conn->get_write_event()->set_handler([this](Event* ev) {
                this->wait_connect_handler(ev);
            });
        Timer::instance()->add_timer(conn->get_write_event(), 60000);
        return SERVX_AGAIN;
    }

    build_request();
    rc = send_request();

    switch (rc) {
    case SERVX_ERROR:
        Logger::instance()->error("send request error");
        conn->close();
        return SERVX_ERROR;
    case SERVX_AGAIN:
        conn->get_read_event()->set_handler(empty_read_handler);
        conn->get_write_event()->set_handler([this](Event* ev) {
                this->send_request_handler(ev);
            });
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

void HttpUpstreamRequest::wait_connect_handler(Event* ev) {
    if (check_timeout(ev, HTTP_BAD_GATEWAY)) {
        return;
    }

    Logger::instance()->debug("connect upstream success");

    build_request();
    int rc = send_request();

    Logger::instance()->debug("send_request return, get %d", rc);

    switch (rc) {
    case SERVX_ERROR:
        close(HTTP_INTERNAL_SERVER_ERROR);
        break;
    case SERVX_AGAIN:
        conn->get_read_event()->set_handler(empty_read_handler);
        conn->get_write_event()->set_handler([this](Event* ev) {
                this->send_request_handler(ev);
            });
        break;
    }
}

void HttpUpstreamRequest::send_request_handler(Event* ev) {
    if (check_timeout(ev, HTTP_INTERNAL_SERVER_ERROR)) {
        return;
    }

    int rc = send_request();
    if (rc == SERVX_ERROR) {
        close(HTTP_INTERNAL_SERVER_ERROR);
    }
}

void HttpUpstreamRequest::recv_response_line_handler(Event* ev) {
    if (check_timeout(ev, HTTP_BAD_GATEWAY)) {
        return;
    }

    if (response_header_buf == nullptr) {
        Logger::instance()->debug("init request header buf");
        response_header_buf = server->get_body_buf();
        header = std::unique_ptr<HttpResponseHeader>(
            new HttpResponseHeader(response_header_buf));
    }

    if (!conn->get_read_event()->is_timer()) {
        // TODO: del timer
        Timer::instance()->add_timer(conn->get_read_event(), 120000);
    }

    int rc = read_response_header();
    if (rc != SERVX_OK) {
        return;
    }

    rc = header->parse_response_line();
    switch (rc) {
    case SERVX_ERROR:
        close(SERVX_ERROR);
        return;
    case SERVX_OK:
        Logger::instance()->debug("parse upstream line success");
        conn->get_read_event()->set_handler([this](Event* ev) {
                this->recv_response_headers_handler(ev);
            });

        if (response_header_buf->get_size() > 0) {
            recv_response_headers_handler(ev);
        }
        return;
    }

    if (check_header_buf_full()) {
        return;
    }
}

void HttpUpstreamRequest::recv_response_headers_handler(Event* ev) {
    if (check_timeout(ev, HTTP_BAD_GATEWAY)) {
        return;
    }

    int rc = read_response_header();
    if (rc != SERVX_OK) {
        return;
    }

    rc = header->parse_headers();
    switch (rc) {
    case SERVX_ERROR:
        close(SERVX_ERROR);
        return;
    case SERVX_OK:
        response_header_done();
        return;
    }

    if (check_header_buf_full()) {
        return;
    }
}

void HttpUpstreamRequest::response_header_done() {
    auto length = header->get_header("content-length");
    if (!length.empty()) {
        content_length = atol(length.c_str());
        if (content_length == 0 && length != "0") {
            close(HTTP_BAD_GATEWAY);
            return;
        }
    }

    Logger::instance()->debug("content-length = %d", content_length);

    if (content_length == 0 || content_length == -1) {
        close(SERVX_OK);
    } else {
        Buffer *buf = server->get_body_buf();
        int size = response_header_buf->get_size();
        if (size > 0) {
            memmove(buf->get_pos(), response_header_buf->get_pos(), size);
            buf->move_last(size);
        }
        response_body_bufs.emplace_back(buf);

        response_header_buf->set_last(response_header_buf->get_pos());
        response_header_buf->set_pos(response_header_buf->get_start());
        response_header_handler(request, response_header_buf);

        conn->get_read_event()->set_handler([this](Event* ev) {
                this->recv_response_body_handler(ev);
            });

        if (size > 0) {
            // TODO: recv body
            recv += size;
            handle_response_body();
        }
    }
}

int HttpUpstreamRequest::handle_response_body() {
    Buffer *buf = response_body_bufs.back();
    int rc = response_body_handler(request, buf);

    if (rc == SERVX_ERROR) {
        Logger::instance()->info("response_body_handler success");
        close(rc);
        return SERVX_ERROR;
    }

    if (recv == content_length) {
        close(SERVX_OK);
        return SERVX_OK;
    }

    if (rc == SERVX_OK) {
        // TODO: discard
    }

    return SERVX_AGAIN;
}

void HttpUpstreamRequest::recv_response_body_handler(Event* ev) {
    Buffer *buf;
    int n, rc, size;

    if (ev->is_timer()) {
        // TODO
        Timer::instance()->del_timer(ev);
    }

    while (true) {
        buf = response_body_bufs.back();
        size = buf->get_remain();
        n = conn->recv_data(buf, size);

        if (n < 0) {
            Logger::instance()->error("get data from upstream error");
            close(HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        if (n == 0) {
            Logger::instance()->info("upstream permaturely closed connection");
            conn->get_read_event()->set_eof(true);
            close(HTTP_BAD_GATEWAY);
            return;
        }

        rc = handle_response_body();
        if (rc != SERVX_AGAIN) {
            return;
        }

        // TODO: maybe discard

        if (n != size) {
            return;
        }
        response_body_bufs.emplace_back(server->get_body_buf());
    }
}

void HttpUpstreamRequest::build_request() {
    // copy it
    request_body_bufs = request->get_request_body()->get_body_buffer();
    request_header_buf = server->get_body_buf();

    int n;
    char *pos = request_header_buf->get_pos();

    n = sprintf(pos, "%s %s%s%s HTTP/1.1\r\n",
        request->get_request_header()->get_method().c_str(),
        request->get_request_header()->get_uri().c_str(),
        request->get_request_header()->get_args().empty() ? "" : "?",
        request->get_request_header()->get_args().c_str());
    pos += n;
    request_header_buf->set_last(pos);

    // TODO: keep-alive
    // TODO: host

    auto &headers = request->get_request_header()->get_headers();
    auto hb = headers.begin();
    auto he = headers.end();

    while (hb != he) {
        // TODO: avoid compare
        if (hb->first != "host" && hb->first != "connection") {
            n = snprintf(pos, request_header_buf->get_remain(), "%s:%s\r\n",
                         hb->first.c_str(), hb->second.c_str());
            if (request_header_buf->get_remain() < 2) {
                // TODO: error handler
                Logger::instance()->info("header too large");
                break;
            }
            pos += n;
            request_header_buf->set_last(pos);
        }
        ++hb;
    }

    pos[0] = '\r';
    pos[1] = '\n';
    request_header_buf->set_last(pos + 2);

    Logger::instance()->debug("build request success");
}

int HttpUpstreamRequest::read_response_header() {
    int rc = conn->recv_data(response_header_buf,
        response_header_buf->get_remain());

    switch (rc) {
    case 0:
        Logger::instance()->info("upstream permaturely closed connection");
        conn->get_read_event()->set_eof(true);
        close(HTTP_BAD_GATEWAY);
        return SERVX_ERROR;
    case SERVX_ERROR:
        Logger::instance()->warn("upstream error");
        close(HTTP_INTERNAL_SERVER_ERROR);
        return SERVX_ERROR;
    case SERVX_AGAIN:
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

int HttpUpstreamRequest::send_request() {
    auto size = request_header_buf->get_size();

    if (size > 0) {
        int n = conn->send_data(request_header_buf, size);

        if (conn->is_error()) {
            return SERVX_ERROR;
        }

        if (static_cast<uint32_t>(n) != size) {
            return SERVX_AGAIN;
        }

        Logger::instance()->debug("send request header success");
    }

    if (!request_body_bufs.empty()) {
        auto first = request_body_bufs.begin();
        auto last = request_body_bufs.end();
        auto iter = conn->send_chain(first, last);

        if (conn->is_error()) {
            return SERVX_ERROR;
        }

        request_body_bufs.erase(first, iter);

        if (iter != last) {
            return SERVX_AGAIN;
        }

        Logger::instance()->debug("send request body success");
    }

    conn->get_read_event()->set_handler([this](Event* ev) {
            this->recv_response_line_handler(ev);
        });
    conn->get_write_event()->set_handler(empty_write_handler);

    return SERVX_OK;
}

void HttpUpstreamRequest::process_line() {
}

void HttpUpstreamRequest::process_headers() {
}

}
