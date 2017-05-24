#include "http_request.h"

#include <cstdlib>

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "http_connection.h"
#include "http_parse.h"
#include "http_phase.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Connection* c)
    : conn(c), http_method(HTTP_METHOD_UNKONWN), parse_state(0),
      body(new HttpRequestBody(this)),
      response(new HttpResponse(c)),
      phase(HTTP_POST_READ_PHASE), phase_index(0),
      content_handler(nullptr), server(nullptr),
      quoted(false), keep_alive(false) {
}

std::string HttpRequest::get_headers(const char* s) const {
    auto iter = headers.find(std::string(s));
    if (iter == headers.end()) {
        return std::string("");
    }
    return iter->second;
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

    if (get_recv_buf()->get_remain() == 0) {
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

    rc = http_parse_request_headers(this);

    if (rc == SERVX_OK) {
        Logger::instance()->debug("parse request headers success!");

        auto host = get_headers("host");
        if (host.empty()) {
            Logger::instance()->error("can not find host");
            finalize(HTTP_BAD_REQUEST);
            return;
        } else {
            HttpConnection *hc = conn->get_context<HttpConnection>();
            server = hc->get_listening()->search_server(host);
            if (server == hc->get_listening()->get_default_server()) {
                Logger::instance()->debug("use default server");
            }
        }

        auto length = get_headers("content-length");
        if (!length.empty()) {
            long n = atol(length.c_str());
            if (n == 0 && length != "0") {
                finalize(HTTP_BAD_REQUEST);
                return;
            }
            body->set_content_length(n);
        }

        auto encoding = get_headers("transfer-encoding");
        if (!encoding.empty() && encoding != "identity") {
            Logger::instance()->error("unknown encoding %s",
                                      encoding.c_str());
            finalize(HTTP_NOT_IMPLEMENTED);
            return;
        }

        auto connection = get_headers("connection");
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

    rc = http_parse_request_line(this);

    if (rc == SERVX_OK) {
        Logger::instance()->debug("parsing request success!\n"   \
                                  "method: %s\n"                 \
                                  "schema: %s\n"                 \
                                  "host: %s\n"                   \
                                  "uri: %s\n"                    \
                                  "args: %s\n"                   \
                                  "version: %s\n",
                                  method.c_str(),
                                  schema.c_str(),
                                  host.c_str(),
                                  uri.c_str(),
                                  args.c_str(),
                                  version.c_str());

        if (version != "1.1") {
            finalize(HTTP_BAD_REQUEST);
            return;
        }

        if (quoted) {
            if (http_parse_quoted(this) == SERVX_ERROR) {
                finalize(HTTP_BAD_REQUEST);
                return;
            }
        }

        if (!args.empty()) {
            if (http_parse_args(this) == SERVX_ERROR) {
                finalize(HTTP_BAD_REQUEST);
                return;
            }
        }

        if (!host.empty()) {
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
    if (status != SERVX_OK) {
        response->set_content_length(0);
        response->set_status(status);
        response->send_header();
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
        conn->get_write_event()->set_handler(http_empty_write_handler);
        hc->close_request();
    }
}

void http_block_reading(HttpRequest* req) {}

void http_block_writing(HttpRequest* req) {}

}
