#include "http_request.h"

#include <cstdlib>

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "http_connection.h"
#include "http_phase.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Connection* c)
    : conn(c), http_method(HTTP_METHOD_UNKONWN),
      header(c->get_recv_buf()), body(this), response(c),
      phase(HTTP_POST_READ_PHASE), phase_index(0),
      server(nullptr), location(nullptr),
      keep_alive(false) {
}

void HttpRequest::handle(Event* ev) {
    // TODO: cancel dalay

    if (ev->is_write_event()) {
        write_handler(this);
    } else {
        read_handler(this);
    }

    // TODO: process subrequest
}

int HttpRequest::read_headers() {
    Event *ev = conn->get_read_event();

    if (conn->get_recv_buf()->get_remain() == 0) {
        // TODO: enlarge double size
        Logger::instance()->error("request too large");
        finalize(HTTP_BAD_REQUEST);
        return SERVX_ERROR;
    }

    int n = conn->recv_data();

    if (n == 0 || n == SERVX_ERROR) {
        if (n == 0) {
            Logger::instance()->
                info("client permaturely closed connection");
        }
        finalize(HTTP_BAD_REQUEST);
        return SERVX_ERROR;
    }

    if (n == SERVX_AGAIN) {
        if (!ev->is_timer()) {
            Timer::instance()->add_timer(ev,
                server->get_core_conf()->client_header_timeout);
        }
        ConnectionPool::instance()->enable_reusable(conn);
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

void HttpRequest::process_headers(Event* ev) {
    if (ev->is_timeout()) {
        Logger::instance()->info("%d client time out", HTTP_REQUEST_TIME_OUT);
        conn->set_timeout(true);
        close(HTTP_REQUEST_TIME_OUT);
        return;
    }

    int rc;

    if (ev->is_ready()) {
        rc = read_headers();
        if (rc != SERVX_OK) {
            return;
        }
    }

    rc = header.parse_request_headers();

    if (rc == SERVX_OK) {
        Logger::instance()->debug("parse request headers success!");

        auto host = header.get_header("host");
        if (host.empty()) {
            Logger::instance()->error("can not find host");
            finalize(HTTP_BAD_REQUEST);
            return;
        } else {
            HttpConnection *hc = conn->get_context<HttpConnection>();
            server = hc->get_listening()->search_server(host);
            response.set_server(server);
            if (server == hc->get_listening()->get_default_server()) {
                Logger::instance()->debug("use default server");
            }
        }

        auto length = header.get_header("content-length");
        if (!length.empty()) {
            long n = atol(length.c_str());
            if (n == 0 && length != "0") {
                finalize(HTTP_BAD_REQUEST);
                return;
            }
            body.set_content_length(n);
        }

        auto encoding = header.get_header("transfer-encoding");
        if (!encoding.empty() && encoding != "identity") {
            Logger::instance()->error("unknown encoding %s",
                                      encoding.c_str());
            finalize(HTTP_NOT_IMPLEMENTED);
            return;
        }

        auto connection = header.get_header("connection");
        if (connection == "keep-alive") {
            Logger::instance()->debug("connection keep-alive");
            set_keep_alive(true);
            get_response()->set_keep_alive(true);
        }

        if (conn->get_read_event()->is_timer()) {
            Timer::instance()->del_timer(conn->get_read_event());
        }

        conn->get_read_event()->set_handler(
            [this](Event* ev) { this->handle(ev); });
        conn->get_write_event()->set_handler(
            [this](Event* ev) { this->handle(ev); });
        set_read_handler(http_block_reading);
        set_write_handler([](HttpRequest* req)
            { HttpPhaseRunner::instance()->run(req); });

        Logger::instance()->debug("prepare to run phase...");

        write_handler(this);
        return;
    }

    if (rc != SERVX_AGAIN) {
        finalize(HTTP_BAD_REQUEST);
    }
}

void HttpRequest::process_line(Event* ev) {
    Connection *conn = ev->get_connection();

    if (ev->is_timeout()) {
        Logger::instance()->info("%d client time out", HTTP_REQUEST_TIME_OUT);
        conn->set_timeout(true);
        close(HTTP_REQUEST_TIME_OUT);
        return;
    }

    int rc;

    if (ev->is_ready()) {
        rc = read_headers();
        if (rc != SERVX_OK) {
            return;
        }
    }

    rc = header.parse_request_line();

    if (rc == SERVX_OK) {
        Logger::instance()->debug("parsing request success!\n"   \
                                  "method: %s\n"                 \
                                  "schema: %s\n"                 \
                                  "host: %s\n"                   \
                                  "uri: %s\n"                    \
                                  "args: %s\n"                   \
                                  "version: %s\n",
                                  header.method.c_str(),
                                  header.schema.c_str(),
                                  header.host.c_str(),
                                  header.uri.c_str(),
                                  header.args.c_str(),
                                  header.version.c_str());

        if (header.version != "1.1") {
            finalize(HTTP_BAD_REQUEST);
            return;
        }

        http_method = header.parse_request_method();

        if (header.quoted) {
        /*    if (http_parse_quoted(this) == SERVX_ERROR) {
                finalize(HTTP_BAD_REQUEST);
                return;
            }*/
        }

        if (!header.args.empty()) {
        /*    if (http_parse_args(this) == SERVX_ERROR) {
                finalize(HTTP_BAD_REQUEST);
                return;
            }*/
        }

        if (!header.host.empty()) {
            // TODO: check host format
        }

        Logger::instance()->debug("prepare to process request headers...");

        ev->set_handler([this](Event* ev) { this->process_headers(ev); });
        process_headers(ev);
        return;
    }

    if (rc != SERVX_AGAIN) {
        finalize(HTTP_BAD_REQUEST);
    }
}

void HttpRequest::finalize(int rc) {
    Logger::instance()->debug("http finalize, rc = %d", rc);

    if (rc == SERVX_AGAIN) {
        return;
    }

    if (rc == SERVX_ERROR) {
        rc = HTTP_INTERNAL_SERVER_ERROR;
    }

    close(rc);
}

void HttpRequest::close(int status) {
    if ((conn->is_error() ||
        conn->is_timeout()) && response.is_sent()) {
        conn->close();
        return;
    }

    if (status != SERVX_OK) {
        response.set_content_length(0);
        response.set_status(status);
        // TODO: maybe again
        response.send_header();
    }

    if (status == HTTP_REQUEST_TIME_OUT ||
        conn->get_read_event()->is_eof() ||
        conn->is_error() ||
        conn->is_timeout()) {
        conn->close();
        return;
    }

    if (!keep_alive) {
        // TODO: linger close
        conn->close();
    } else {
        Timer::instance()->add_timer(conn->get_read_event(), 120000);
        HttpConnection *hc = conn->get_context<HttpConnection>();
        conn->get_recv_buf()->shrink();
        conn->get_read_event()->set_ready(false);
        conn->get_read_event()->set_handler([hc](Event* ev)
            { hc->wait_request(ev); });
        conn->get_write_event()->set_ready(false);
        conn->get_write_event()->set_handler(empty_write_handler);
        hc->close_request();
    }
}

void http_block_reading(HttpRequest* req) {}

void http_block_writing(HttpRequest* req) {}

}
