#include "http_request_body.h"

#include "core.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequestBody::~HttpRequestBody() {
    // we can ensure 'req' is not freed here normally
    for (Buffer *buf : body_buffer) {
        req->get_server()->ret_body_buf(buf);
    }
}

int HttpRequestBody::read(const http_req_handler_t& h) {
    // TODO: chunked
    if (content_length <= 0) {
        return h(req);
    }

    handler = h;
    body_buffer.emplace_back(req->get_server()->get_body_buf());

    int pre = req->get_recv_buf()->get_size();
    if (pre != 0) {
        Buffer *buf = body_buffer.back();
        if (pre > content_length) {
            pre = content_length;
        }
        // TODO: don't copy it
        memmove(buf->get_pos(), req->get_recv_buf()->get_pos(), pre);
        req->get_recv_buf()->move_pos(pre);
        recv += pre;
    }

    if (recv == content_length) {
        req->set_read_handler(http_block_reading);
        return handler(req);
    }

    req->set_read_handler(read_request_body_handler);
    return handle_read();
}

int HttpRequestBody::handle_read() {
    Connection *conn = req->get_connection();
    int rc, size, left;

    if (!conn->get_read_event()->is_timer()) {
        // TODO: del timer
        Timer::instance()->add_timer(conn->get_read_event(),
            req->get_server()->get_core_conf()->client_body_timeout);
    }

    while (true) {
        Buffer *buf = body_buffer.back();

        left = content_length - recv;
        size = buf->get_remain() >
            static_cast<uint32_t>(left) ? left : buf->get_remain();
        rc = conn->recv_data(buf, size);

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
            if (conn->get_read_event()->is_timer()) {
                Timer::instance()->del_timer(conn->get_read_event());
            }
            req->set_read_handler(http_block_reading);
            return handler(req);
        } else if (buf->get_remain() == 0) {
            body_buffer.emplace_back(req->get_server()->get_body_buf());
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

    int pre = req->get_recv_buf()->get_size();
    if (pre != 0) {
        if (pre > content_length) {
            pre = content_length;
        }
        req->get_recv_buf()->move_pos(pre);
        recv += pre;
    }

    if (recv == content_length) {
        req->set_read_handler(http_block_reading);
        return SERVX_OK;
    }

    body_buffer.emplace_back(req->get_server()->get_body_buf());
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
            req->set_read_handler(http_block_reading);
            req->get_server()->ret_body_buf(buf);
            body_buffer.pop_back();
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
