#include "http_request_body.h"

#include "core.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

int HttpRequestBody::read(const http_req_handler_t& h) {
    handler = h;

    if (content_length <= 0) {
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
        req->set_read_handler(http_block_reading);
        handler(req);
        return SERVX_OK;
    }

    if (recv > content_length) {
        Logger::instance()->error("data is more than content_length");
        return SERVX_ERROR;
    }

    req->set_read_handler(read_request_body_handler);
    return handle_read();
}

int HttpRequestBody::handle_read() {
    Connection *conn = req->get_connection();
    int rc, size;

    while (true) {
        Buffer &buf = body.back();

        size = buf.get_remain() > content_length - recv ?
            content_length - recv : buf.get_remain();
        rc = conn->recv_data(&buf, size);

        if (rc == SERVX_ERROR) {
            return SERVX_ERROR;
        }

        if (rc == SERVX_AGAIN) {
            if (!conn->get_read_event()->is_timer()) {
                Timer::instance()->add_timer(conn->get_read_event(), 60000);
            }
            return SERVX_AGAIN;
        }

        if (rc == 0) {
            Logger::instance()->info("client permaturely closed connection");
            conn->get_read_event()->set_eof(true);
            return SERVX_ERROR;
        }

        buf.set_last(buf.get_pos() + rc);
        recv += rc;
        if (recv == content_length) {
            req->set_read_handler(http_block_reading);
            handler(req);
            return SERVX_OK;
        } else if (buf.get_remain() == 0) {
            body.emplace_back(4096);
        } else {
            if (!conn->get_read_event()->is_timer()) {
                Timer::instance()->add_timer(conn->get_read_event(), 60000);
            }
            return SERVX_AGAIN;
        }
    }
}

int HttpRequestBody::discard() {
    if (discard_buffer != nullptr) {
        return SERVX_OK;
    }

    Event *ev = req->get_connection()->get_read_event();
    if (ev->is_timer()) {
        Timer::instance()->del_timer(ev);
    }

    if (content_length < 0) {
        return SERVX_OK;
    }

    discard_buffer = std::unique_ptr<Buffer>(new Buffer(1024));
    int rc = handle_discard();

    if (rc == SERVX_ERROR) {
        return SERVX_ERROR;
    }

    return SERVX_OK;
}

int HttpRequestBody::handle_discard() {
    Connection *conn = req->get_connection();
    int rc;

    while (true) {
        rc = conn->recv_data(discard_buffer.get(), 1024);
        discard_buffer->reset();

        if (rc < 0) {
            return rc;
        }

        recv += rc;
        if (recv == content_length) {
            discard_buffer = nullptr;
            discarded = 1;
            return SERVX_OK;
        }

        if (rc != 1024) {
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
    }

    int rc = req->get_request_body()->handle_read();

    if (rc == SERVX_ERROR) {
        req->finalize(SERVX_ERROR);
    }
}

}
