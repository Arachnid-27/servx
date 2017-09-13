#include "http_request_body.h"

#include "core.h"
#include "http_request.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

int HttpRequestBody::read(const http_req_handler_t& h) {
    // TODO: chunked
    if (content_length <= 0) {
        return h(req);
    }

    handler = h;
    body_buffer.push_back(req->get_buffer());

    Buffer *pre_buf = req->get_connection()->get_recv_buf();
    int pre = pre_buf->get_size();
    if (pre != 0) {
        Buffer *buf = body_buffer.back();
        if (pre > content_length) {
            pre = content_length;
        }
        // TODO: don't copy it
        memmove(buf->get_pos(), pre_buf->get_pos(), pre);
        pre_buf->move_pos(pre);
        buf->move_last(pre);
        recv += pre;
    }

    if (recv == content_length) {
        req->set_read_handler(HttpRequest::http_block_reading);
        return handler(req);
    }

    req->set_read_handler(read_request_body_handler);
    return handle_read();
}

int HttpRequestBody::handle_read() {
    Connection *conn = req->get_connection();
    int rc, size, left;

    if (!conn->get_read_event()->is_timer()) {
        Timer::instance()->add_timer(conn->get_read_event(),
            req->get_server()->get_timeout());
    }

    while (true) {
        Buffer *buf = body_buffer.back();

        left = content_length - recv;
        size = buf->get_remain() >
            static_cast<uint32_t>(left) ? left : buf->get_remain();
        rc = conn->recv_data(buf, size);

        if (rc < 0) {
            if (conn->get_read_event()->is_timer()) {
                Timer::instance()->del_timer(conn->get_read_event());
            }
            return rc;
        }

        recv += rc;

        if (recv != content_length && conn->get_read_event()->is_eof()) {
            if (conn->get_read_event()->is_timer()) {
                Timer::instance()->del_timer(conn->get_read_event());
            }
            Logger::instance()->info("client permaturely closed connection");
            return SERVX_ERROR;
        }

        if (recv == content_length) {
            if (conn->get_read_event()->is_timer()) {
                Timer::instance()->del_timer(conn->get_read_event());
            }
            req->set_read_handler(HttpRequest::http_block_reading);
            return handler(req);
        } else if (buf->get_remain() == 0) {
            body_buffer.push_back(req->get_buffer());
        } else {
            return SERVX_AGAIN;
        }
    }
}

int HttpRequestBody::discard() {
    if (discarded || !body_buffer.empty() || content_length < 0) {
        return SERVX_OK;
    }

    Event *ev = req->get_connection()->get_read_event();
    if (ev->is_timer()) {
        Timer::instance()->del_timer(ev);
    }

    Buffer *pre_buf = req->get_connection()->get_recv_buf();
    int pre = pre_buf->get_size();
    if (pre != 0) {
        if (pre > content_length) {
            pre = content_length;
        }
        pre_buf->move_pos(pre);
        recv += pre;
    }

    if (recv == content_length) {
        req->set_read_handler(HttpRequest::http_block_reading);
        return SERVX_OK;
    }

    body_buffer.push_back(req->get_buffer());
    int rc = handle_discard();

    if (rc == SERVX_ERROR) {
        Logger::instance()->error("handle discard error");
        return SERVX_ERROR;
    }

    return SERVX_OK;
}

int HttpRequestBody::handle_discard() {
    Buffer *buf = body_buffer.back();
    Connection *conn = req->get_connection();
    int rc;

    while (true) {
        rc = conn->recv_data(buf, buf->get_size());
        buf->reset();

        if (rc < 0) {
            return rc;
        }

        if (rc == 0) {
            Logger::instance()->info("client permaturely closed connection");
            conn->get_read_event()->set_eof(true);
            return SERVX_ERROR;
        }

        recv += rc;
        if (recv == content_length) {
            req->set_read_handler(HttpRequest::http_block_reading);
            discarded = 1;
            return SERVX_OK;
        }

        if (static_cast<uint32_t>(rc) != buf->get_size()) {
            return SERVX_AGAIN;
        }
    }
}

void HttpRequestBody::discard_request_body_handler(HttpRequest* req) {
    int rc = req->get_request_body()->handle_discard();

    if (rc == SERVX_ERROR) {
        req->finalize(SERVX_ERROR);
    }
}

void HttpRequestBody::read_request_body_handler(HttpRequest* req) {
    if (req->get_connection()->is_timeout()) {
        req->get_connection()->set_timeout(true);
        req->finalize(HTTP_REQUEST_TIME_OUT);
        return;
    }

    int rc = req->get_request_body()->handle_read();

    if (rc == SERVX_ERROR) {
        req->finalize(SERVX_ERROR);
    }
}

}
