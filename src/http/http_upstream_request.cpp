#include "http_upstream_request.h"

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "http.h"
#include "logger.h"
#include "timer.h"

namespace servx {

void HttpUpstreamRequest::close(int rc) {
    // TODO: keep-alive
    finished = true;
    Logger::instance()->debug("close http upstream request, rc = %d", rc);
    finalize_handler(request, rc);
    if (conn != nullptr) {
        conn->close();
    }
}

int HttpUpstreamRequest::connect() {
    auto socket = server->get_socket();

    int rc = socket->connect();
    if (rc == SERVX_ERROR) {
        Logger::instance()->warn("connect error");
        return SERVX_ERROR;
    }

    Logger::instance()->debug("connect fd = %d", socket->get_fd());

    conn = ConnectionPool::instance()
        ->get_connection(socket->get_fd(), false);

    if (conn == nullptr) {
        Logger::instance()->warn("get connection error");
        socket->close();
        return SERVX_ERROR;
    }

    socket->release();

    if (!add_event(conn->get_write_event())) {
        Logger::instance()->warn("add connection error");
        conn->close();
        return SERVX_ERROR;
    }

    if (rc == SERVX_AGAIN) {
        Logger::instance()->debug("connect again");
        conn->get_read_event()->set_handler(Event::empty_read_handler);
        conn->get_write_event()->set_handler([this](Event* ev) {
                this->wait_connect_handler(ev);
            });
        Timer::instance()->add_timer(conn->get_write_event(), 60000);
        return SERVX_AGAIN;
    }

    if (build_request() == SERVX_ERROR) {
        conn->close();
        return SERVX_ERROR;
    }

    rc = send_request();

    switch (rc) {
    case SERVX_ERROR:
        Logger::instance()->warn("send request error");
        conn->close();
        return SERVX_ERROR;
    case SERVX_AGAIN:
        conn->get_read_event()->set_handler(Event::empty_read_handler);
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

    if (build_request() == SERVX_ERROR) {
        close(HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    int rc = send_request();
    switch (rc) {
    case SERVX_ERROR:
        Logger::instance()->warn("send request error");
        close(HTTP_INTERNAL_SERVER_ERROR);
        break;
    case SERVX_AGAIN:
        conn->get_read_event()->set_handler(Event::empty_read_handler);
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

int HttpUpstreamRequest::build_request() {
    // copy it
    request_body_bufs = request->get_request_body()->get_body_buffer();
    request_header_buf = request->get_buffer();

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
            pos += n;
            request_header_buf->set_last(pos);
            if (request_header_buf->get_remain() < 2) {
                Logger::instance()->warn("request header too large");
                return SERVX_ERROR;
            }
        }
        ++hb;
    }

    pos[0] = '\r';
    pos[1] = '\n';
    request_header_buf->set_last(pos + 2);

    Logger::instance()->debug("build request success");
    return SERVX_OK;
}

int HttpUpstreamRequest::send_request() {
    auto size = request_header_buf->get_size();

    if (size > 0) {
        int n = conn->send_data(request_header_buf, size);

        if (conn->is_error()) {
            Logger::instance()->warn("send data error");
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
            response.recv_response_line_handler(ev);
        });
    conn->get_write_event()->set_handler(Event::empty_write_handler);

    if (!add_event(conn->get_read_event()) ||
        !del_event(conn->get_write_event())) {
        Logger::instance()->warn(
            "add read event, or del write event failed");
        return SERVX_ERROR;
    }

    return SERVX_OK;
}

}
