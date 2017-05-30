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
    finalize_handler(rc);
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
        rc = response_handler(buf);

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

int HttpUpstreamRequest::send_request() {
    int rc = conn->send_chain(request_bufs);

    if (rc == SERVX_OK) {
        conn->get_read_event()->set_handler([this](Event* ev) {
                this->recv_response_handler(ev);
            });
        conn->get_write_event()->set_handler(empty_write_handler);
    }

    return rc;
}

}
