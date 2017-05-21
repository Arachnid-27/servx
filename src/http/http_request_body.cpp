#include "http_request_body.h"

#include "core.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

int HttpRequestBody::read(const http_req_handler_t& h) {
    handler = h;

    if (content_length <= 0/* && !chunked */) {
        handler(req);
        return SERVX_OK;
    }

    // TODO: chunked
    int pre = req->get_recv_buf()->get_size();

    if (pre != 0) {
        body.emplace_back(req->get_recv_buf()->get_pos(), pre, true);
        req->get_recv_buf()->reset();
        recv += pre;
    }

    if (recv == content_length) {
        handler(req);
        return SERVX_OK;
    }

    if (recv > content_length) {
        Logger::instance()->error("data is more than content_length");
        return SERVX_ERROR;
    }

    req->set_read_handler(read_request_body_handler);
    return read();
}

int HttpRequestBody::read() {
    Connection *conn = req->get_connection();
    int rc, size;

    while (true) {
        Buffer *buf = &body.back();

        if (buf->get_remain() == 0) {
            size = content_length - recv;
            body.emplace_back(size >= 4096 ? 4096 : size);
            buf = &body.back();
        }

        size = buf->get_size();
        rc = conn->recv_data(buf);
        recv += (buf->get_size() - size);

        switch (rc) {
        case SERVX_AGAIN:
        case SERVX_PARTIAL:
            if (!conn->get_read_event()->is_timer()) {
                Timer::instance()->add_timer(conn->get_read_event(), 60000);
            }
            return SERVX_AGAIN;
        case SERVX_ERROR:
            return SERVX_ERROR;
        }

        if (recv == content_length) {
            return SERVX_OK;
        }
    }
}

int HttpRequestBody::discard() {
    if (discarded) {
        return SERVX_OK;
    }

    Event *ev = req->get_connection()->get_read_event();
    if (ev->is_timer()) {
        Timer::instance()->del_timer(ev);
    }

    if (content_length <= 0 && !chunked) {
        return SERVX_OK;
    }

    // TODO: recv data && set event handler

    discarded = 1;

    return SERVX_OK;
}

void HttpRequestBody::read_request_body_handler(HttpRequest* req) {
    if (req->get_connection()->is_timeout()) {
        req->get_connection()->set_timeout(true);
        req->finalize(HTTP_REQUEST_TIME_OUT);
    }

    int rc = req->get_request_body()->read();

    if (rc == SERVX_ERROR) {
        req->finalize(SERVX_ERROR);
    }
}

}
