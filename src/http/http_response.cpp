#include "http_response.h"

#include <cstdio>
#include <sys/time.h>

#include "clock.h"
#include "core.h"
#include "http_request.h"
#include "io.h"
#include "logger.h"

namespace servx {

std::string HttpResponse::status_lines[] = {
    "100 Continue",

    "200 OK",
    "206 Partial Content",

    "301 Moved Permanently",
    "302 Moved Temporarily",
    "304 Not Modified",

    "400 Bad Request",
    "401 Unauthorized",
    "402 Forbidden",
    "404 Not Found",
    "405 Not Allowed",
    "408 Request Time-out",
    "409 Conflict",
    "411 Length Required",
    "412 Precondition Failed",
    "413 Request Entity Too Large",
    "414 Request-URI Too Large",

    "500 Internal Server Error",
    "501 Not Implemented",
    "502 Bad Gateway",
    "503 Service Temporarily Unavailable",
    "504 Gateway Time-out"
};

HttpResponse::HttpResponse(HttpRequest* r)
    : request(r), content_length(-1),
      last_modified_time(-1), status(-1),
      header_only(false), chunked(false),
      keep_alive(false), etag(false) {
}

HttpResponse::~HttpResponse() {
    std::for_each(out.begin(), out.end(), [this](Sendable& s)
        { std::for_each(s.chain.begin(), s.chain.end(), [this](Buffer* b)
            { request->get_server()->ret_buffer(b); }); });
}

int HttpResponse::send_header() {
    if (status == -1) {
        return SERVX_ERROR;
    }

    if (status != HTTP_OK &&
        status != HTTP_PARTIAL_CONTENT &&
        status != HTTP_NOT_MODIFIED) {
        last_modified_time = -1;
    }

    if (status == HTTP_NOT_MODIFIED) {
        header_only = 1;
    }

    out.emplace_back();

    int n;
    Buffer *buf = request->get_server()->get_buffer();
    std::list<Buffer*> &chain = out.back().chain;
    chain.emplace_back(buf);
    char *pos = buf->get_pos();

    n = sprintf(pos, "HTTP/1.1 %s\r\n", status_lines[status - 10].c_str());
    pos += n;

    n = sprintf(pos, "Server:servx/0.1\r\n");
    pos += n;

    n = sprintf(pos, "Date:%s\r\n",
                Clock::instance()->get_current_http_time().c_str());
    pos += n;

    if (content_length != -1) {
        n = sprintf(pos, "Content-Length:%ld\r\n", content_length);
        pos += n;
    }

    if (last_modified_time != -1) {
        n = sprintf(pos, "Last-Modified:");
        pos += n;
        n = Clock::format_http_time(last_modified_time, pos);
        if (n == -1) {
            Logger::instance()->warn("format last modified time error");
        } else {
            pos += n;
        }
        n = sprintf(pos, "\r\n");
        pos += n;
    }

    if (etag) {
        if (last_modified_time == -1 || content_length == -1) {
            Logger::instance()->warn("can not set etag");
        } else {
            n = sprintf(pos, "Etag:\"%lx-%lx\"\r\n",
                        last_modified_time, content_length);
            pos += n;
        }
    }

    if (chunked) {
        n = sprintf(pos, "Transfer-Encoding:chunked\r\n");
        pos += n;
    }

    if (keep_alive) {
        n = sprintf(pos, "Connection:keep-alive\r\n");
        pos += n;
    } else {
        n = sprintf(pos, "Connection:close\r\n");
        pos += n;
    }

    buf->set_last(pos);

    for (auto &s : headers) {
        n = snprintf(pos, buf->get_remain(), "%s:%s\r\n",
                     s.first.c_str(), s.second.c_str());
        pos += n;
        buf->set_last(pos);
        if (buf->get_remain() < 2) {
            Logger::instance()->warn("response header too large");
            return SERVX_ERROR;
        }
    }

    pos[0] = '\r';
    pos[1] = '\n';
    buf->set_last(pos + 2);

    return send();
}

int HttpResponse::send_body(std::unique_ptr<File>&& p) {
    if (out.empty()) {
        out.emplace_back();
    }
    std::list<std::unique_ptr<File>> &files = out.back().files;
    files.emplace_back(std::move(p));
    return send();
}

int HttpResponse::send_body(std::list<Buffer*>&& chain) {
    if (!out.back().files.empty()) {
        out.emplace_back();
    }
    auto &l = out.back().chain;
    l.splice(l.end(), std::move(chain));
    return send();
}

int HttpResponse::send() {
    int rc;

    while (!out.empty()) {
        auto &chain = out.front().chain;

        if (!chain.empty()) {
            auto first = chain.begin();
            auto last = chain.end();
            auto iter = request->get_connection()->send_chain(first, last);
            if (request->get_connection()->is_error()) {
                Logger::instance()->info("connection error");
                return SERVX_ERROR;
            }

            sent = 1;
            std::for_each(first, iter,
                [this](Buffer* buf)
                { request->get_server()->ret_buffer(buf); });
            chain.erase(first, iter);

            if (iter != last) {
                return SERVX_AGAIN;
            }
        }

        auto &files = out.front().files;

        if (!files.empty()) {
            if (request->get_location()->is_send_file()) {
                auto iter = files.begin();
                while (!files.empty()) {
                    rc = request->get_connection()->send_file(iter->get());
                    if (rc == SERVX_ERROR) {
                        return SERVX_ERROR;
                    }
                    if (rc != SERVX_OK) {
                        return SERVX_AGAIN;
                    }
                    iter = files.erase(iter);
                }
            } else {
                chain.emplace_back(request->get_server()->get_buffer());
                auto iter = files.begin();
                while (!files.empty()) {
                    if (!(*iter)->file_status()) {
                        return SERVX_ERROR;
                    }

                    rc = io_read((*iter)->get_fd(), chain.back()->get_last(),
                        chain.back()->get_remain());

                    if (rc < 0) {
                        return rc;
                    }

                    if (rc == 0) {
                        return SERVX_ERROR;
                    }

                    int bytes = (*iter)->get_offset() + rc;
                    (*iter)->set_offset(bytes);
                    chain.back()->move_last(rc);

                    if (bytes == (*iter)->get_file_size()) {
                        iter = files.erase(iter);
                        continue;
                    } else if (chain.back()->get_remain() == 0) {
                        chain.emplace_back(request->get_server()->get_buffer());
                        continue;
                    } else {
                        break;
                    }
                }
                continue;
            }
        }

        out.pop_front();
    }

    Logger::instance()->debug("send response success!");

    return SERVX_OK;
}

void HttpResponse::send_response_handler(HttpRequest* r) {
    int rc = r->get_response()->send();

    if (rc == SERVX_ERROR) {
        r->close(SERVX_ERROR);
    }
}

}
