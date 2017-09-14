#include "http_upstream_response.h"

#include "http_upstream_request.h"
#include "logger.h"
#include "timer.h"

namespace servx {

void HttpUpstreamResponse::recv_response_line_handler(Event* ev) {
    if (req->check_timeout(ev, HTTP_BAD_GATEWAY)) {
        return;
    }

    if (response_header_buf == nullptr) {
        response_header_buf = req->request->get_buffer();
        header = std::unique_ptr<HttpResponseHeader>(
            new HttpResponseHeader(response_header_buf));
    }

    if (!req->conn->get_read_event()->is_timer()) {
        // TODO: del timer
        Timer::instance()->add_timer(req->conn->get_read_event(), 120000);
    }

    int rc = read_response_header(ev->get_connection());
    if (rc != SERVX_OK) {
        return;
    }

    rc = header->parse_response_line();
    switch (rc) {
    case SERVX_ERROR:
        req->close(SERVX_ERROR);
        return;
    case SERVX_OK:
        Logger::instance()->debug("parse upstream line success");
        req->conn->get_read_event()->set_handler([this](Event* ev) {
                this->recv_response_headers_handler(ev);
            });

        if (response_header_buf->get_size() > 0) {
            recv_response_headers(ev);
        }
        return;
    }

    if (check_header_buf_full()) {
        return;
    }
}

void HttpUpstreamResponse::recv_response_headers_handler(Event* ev) {
    if (req->check_timeout(ev, HTTP_BAD_GATEWAY)) {
        return;
    }

    int rc = read_response_header(ev->get_connection());

    if (rc != SERVX_OK) {
        return;
    }

    recv_response_headers(ev);
}

void HttpUpstreamResponse::recv_response_headers(Event* ev) {
    int rc = header->parse_headers();
    switch (rc) {
    case SERVX_ERROR:
        req->close(SERVX_ERROR);
        return;
    case SERVX_OK:
        response_header_done(ev);
        return;
    }

    if (check_header_buf_full()) {
        return;
    }
}

void HttpUpstreamResponse::response_header_done(Event* ev) {
    auto length = header->get_header("content-length");
    if (!length.empty()) {
        content_length = atol(length.c_str());
        if (content_length == 0 && length != "0") {
            req->close(HTTP_BAD_GATEWAY);
            return;
        }
    }

    int rc;

    Logger::instance()->debug("response header done, content-length = %d",
        content_length);

    if (content_length == 0 || content_length == -1) {
        response_header_buf->set_last(response_header_buf->get_pos());
        response_header_buf->set_pos(response_header_buf->get_start());
        rc = response_header_handler(req->request, response_header_buf);

        req->close(rc);
    } else {
        Buffer *buf = req->request->get_buffer();
        int size = response_header_buf->get_size();
        if (size > 0) {
            memmove(buf->get_pos(), response_header_buf->get_pos(), size);
            buf->move_last(size);
        }
        response_body_bufs.push_back(buf);

        response_header_buf->set_last(response_header_buf->get_pos());
        response_header_buf->set_pos(response_header_buf->get_start());
        rc = response_header_handler(req->request, response_header_buf);

        if (rc == SERVX_ERROR) {
            Logger::instance()->warn("response_header_header error");
            req->close(rc);
            return;
        }

        req->conn->get_read_event()->set_handler([this](Event* ev) {
                this->recv_response_body_handler(ev);
            });

        if (size > 0) {
            recv += size;
            if (ev->is_ready()) {
                recv_response_body_handler(ev);
            } else {
                handle_response_body();
            }
        }
    }
}

int HttpUpstreamResponse::handle_response_body() {
    Buffer *buf = response_body_bufs.back();
    int rc = response_body_handler(req->request, buf);

    if (rc == SERVX_ERROR) {
        Logger::instance()->warn("response_body_handler error");
        req->close(rc);
        return SERVX_ERROR;
    }

    if (recv == content_length) {
        req->close(SERVX_OK);
        return SERVX_OK;
    }

    return SERVX_AGAIN;
}

void HttpUpstreamResponse::recv_response_body_handler(Event* ev) {
    Buffer *buf;
    int n, rc, size;

    if (ev->is_timer()) {
        // TODO
        Timer::instance()->del_timer(ev);
    }

    while (true) {
        buf = response_body_bufs.back();
        size = buf->get_remain();
        // FIXME conn will be null
        n = ev->get_connection()->recv_data(buf, size);

        if (n < 0) {
            if (n != SERVX_AGAIN) {
                Logger::instance()->warn("get data from upstream error");
                req->close(HTTP_INTERNAL_SERVER_ERROR);
            }
            return;
        }

        if (n == 0) {
            Logger::instance()->info(
                "recv response body, upstream permaturely closed connection");
            ev->get_connection()->get_read_event()->set_eof(true);
            req->close(HTTP_BAD_GATEWAY);
            return;
        }

        recv += n;
        rc = handle_response_body();
        if (rc != SERVX_AGAIN) {
            return;
        }

        if (n != size) {
            return;
        }
        response_body_bufs.emplace_back(req->request->get_buffer());
    }
}

int HttpUpstreamResponse::read_response_header(Connection* conn) {
    int rc = conn->recv_data(response_header_buf,
        response_header_buf->get_remain());

    switch (rc) {
    case 0:
        Logger::instance()->info(
            "read response header, upstream permaturely closed connection");
        conn->get_read_event()->set_eof(true);
        req->close(HTTP_BAD_GATEWAY);
        return SERVX_ERROR;
    case SERVX_ERROR:
        Logger::instance()->warn("upstream error, errno %d", errno);
        req->close(HTTP_INTERNAL_SERVER_ERROR);
        return SERVX_ERROR;
    case SERVX_AGAIN:
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

bool HttpUpstreamResponse::check_header_buf_full() {
    if (response_header_buf->get_remain() == 0) {
        Logger::instance()->error("response header too large");
        req->close(SERVX_ERROR);
        return true;
    }
    return false;
}

}
