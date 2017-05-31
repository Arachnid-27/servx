#include "http_upstream_request.h"

#include "connection_pool.h"
#include "core.h"
#include "http.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpUpstreamRequest::~HttpUpstreamRequest() {
    for (Buffer *buf : response_bufs) {
        server->ret_body_buf(buf);
    }
}

void HttpUpstreamRequest::close(int rc) {
    // TODO: keep-alive
    finalize_handler(request, rc);
    conn->close();
}

int HttpUpstreamRequest::connect() {
    auto socket = server->get_socket();
    int rc = socket->connect();

    if (rc == SERVX_ERROR) {
        return SERVX_ERROR;
    }

    conn = ConnectionPool::instance()
        ->get_connection(socket->get_fd(), false);

    if (rc == SERVX_AGAIN) {
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
    if (ev->is_timeout()) {
        Logger::instance()->warn("upstream timeout");
        ev->get_connection()->set_timeout(true);
        close(HTTP_GATEWAY_TIME_OUT);
        return;
    }

    build_request();
    int rc = send_request();

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
    if (ev->is_timeout()) {
        Logger::instance()->warn("upstream timeout");
        ev->get_connection()->set_timeout(true);
        close(HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    int rc = send_request();

    if (rc == SERVX_ERROR) {
        close(HTTP_INTERNAL_SERVER_ERROR);
    }
}

void HttpUpstreamRequest::recv_response_handler(Event* ev) {
    if (ev->is_timeout()) {
        Logger::instance()->warn("upstream timeout");
        ev->get_connection()->set_timeout(true);
        close(HTTP_BAD_GATEWAY);
        return;
    }

    response_bufs.emplace_back(server->get_body_buf());

    int n, rc, size;

    while (true) {
        n = conn->recv_data();

        Buffer *buf = response_bufs.back();

        size = buf->get_remain();
        n = conn->recv_data(buf, size);

        if (n < 0) {
            close(HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        if (n == 0) {
            Logger::instance()->info("upstream permaturely closed connection");
            conn->get_read_event()->set_eof(true);
            close(HTTP_BAD_GATEWAY);
            return;
        }

        // TODO: header and body handler
        rc = response_handler(request, buf);

        if (rc == SERVX_OK || rc == SERVX_ERROR) {
            close(rc);
            return;
        }

        if (n == size) {
            response_bufs.emplace_back(server->get_body_buf());
            continue;
        }

        if (!conn->get_read_event()->is_timer()) {
            // TODO: del timer
            Timer::instance()->add_timer(conn->get_read_event(), 120000);
            break;
        }
    }
}

void HttpUpstreamRequest::build_request() {
    // copy it
    request_body_bufs = request->get_request_body()->get_body_buffer();
    request_header_bufs.emplace_back(server->get_body_buf());

    int n;
    Buffer *buf = request_header_bufs.back();
    char *pos = buf->get_pos();

    n = sprintf(pos, "%s %s%c%s HTTP/1.1\r\n",
        request->get_method().c_str(), request->get_uri().c_str(),
        request->get_args().empty() ? ' ' : '?',
        request->get_args().c_str());
    pos += n;

    // TODO: keep-alive
    // TODO: host

    auto hb = request->headers_begin();
    auto he = request->headers_end();

    while (hb != he) {
        // TODO: avoid compare
        if (hb->first != "host" && hb->first != "connection") {
            n = snprintf(pos, buf->get_remain(), "%s:%s\r\n",
                         hb->first.c_str(), hb->second.c_str());
            if (pos + n == buf->get_end()) {
                request_header_bufs.emplace_back(server->get_body_buf());
                buf = request_header_bufs.back();
                pos = buf->get_pos();
                n = snprintf(pos, buf->get_remain(), "%s:%s\r\n",
                             hb->first.c_str(), hb->second.c_str());
            }
            pos += n;
            buf->set_last(pos);
            ++hb;
        }
    }

    if (buf->get_remain() < 2) {
        request_header_bufs.emplace_back(server->get_body_buf());
        buf = request_header_bufs.back();
        pos = buf->get_pos();
    }

    pos[0] = '\r';
    pos[1] = '\n';
}

int HttpUpstreamRequest::send_request() {
    std::list<Buffer*>::iterator first, last, iter;

    if (!request_header_bufs.empty()) {
        first = request_header_bufs.begin();
        last = request_header_bufs.end();
        iter = conn->send_chain(first, last);

        if (conn->is_error()) {
            return SERVX_ERROR;
        }

        std::for_each(first, iter,
            [this](Buffer* buf) { server->ret_body_buf(buf); });
        request_header_bufs.erase(first, iter);

        if (iter != last) {
            return SERVX_AGAIN;
        }
    }

    first = request_body_bufs.begin();
    last = request_body_bufs.end();
    iter = conn->send_chain(first, last);

    if (conn->is_error()) {
        return SERVX_ERROR;
    }

    request_header_bufs.erase(first, iter);

    if (iter != last) {
        return SERVX_AGAIN;
    }

    conn->get_read_event()->set_handler([this](Event* ev) {
            this->recv_response_handler(ev);
        });
    conn->get_write_event()->set_handler(empty_write_handler);

    return SERVX_OK;
}

}
