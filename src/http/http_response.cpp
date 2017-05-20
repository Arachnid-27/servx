#include "http_response.h"

#include <cstdio>
#include <sys/time.h>

#include "clock.h"
#include "core.h"
#include "http_request.h"
#include "io.h"
#include "logger.h"

namespace servx {

std::unordered_map<int, std::string> HttpResponse::status_lines = {
    { HTTP_CONTINUE, "Continue" },

    { HTTP_OK, "OK" },
    { HTTP_PARTIAL_CONTENT, "Partial Content" },

    { HTTP_MOVED_PERMANENTLY, "Moved Permanently" },
    { HTTP_MOVED_TEMPORARILY, "Moved Temporarily" },
    { HTTP_NOT_MODIFIED, "Not Modified" },

    { HTTP_BAD_REQUEST, "Bad Request" },
    { HTTP_UNAUTHORIZED, "Unauthorized" },
    { HTTP_FORBIDDEN, "Forbidden" },
    { HTTP_NOT_FOUND, "Not Found" },
    { HTTP_NOT_ALLOWED, "Not Allowed" },
    { HTTP_REQUEST_TIME_OUT, "Request Time-out" },
    { HTTP_CONFLICT, "Conflict" },
    { HTTP_LENGTH_REQUIRED, "Length Required" },
    { HTTP_PRECONDITION_FAILED, "Precondition Failed" },
    { HTTP_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large" },
    { HTTP_REQUEST_URI_TOO_LARGE, "Request-URI Too Large" },

    { HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error" },
    { HTTP_NOT_IMPLEMENTED, "Not Implemented" },
    { HTTP_BAD_GATEWAY, "Bad Gateway" },
    { HTTP_SERVICE_UNAVAILABLE, "Service Temporarily Unavailable" },
    { HTTP_GATEWAY_TIME_OUT, "Gateway Time-out" }
};

HttpResponse::HttpResponse(Connection* c)
    : conn(c), content_length(-1),
      last_modified_time(-1), status(-1) {
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

    std::list<Buffer> &chain = out.back().chain;
    chain.emplace_back(2048);

    int n;
    Buffer *buf = &chain.back();
    char *pos = buf->get_pos();

    n = sprintf(pos, "HTTP/1.1 %d %s\r\n",
                status, status_lines[status].c_str());
    pos += n;

    n = sprintf(pos, "Server:servx/0.1\r\n");
    pos += n;

    n = sprintf(pos, "Date:%s\r\n",
                Clock::instance()->get_current_http_time().c_str());

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
        // buffer full means that we need alloc a new buffer
        // but we should discard the last result (don't invoke set_last)
        // becase we don't know if it is truncated
        if (pos + n == buf->get_end()) {
            chain.emplace_back(2048);
            buf = &chain.back();
            pos = buf->get_pos();
            n = snprintf(pos, buf->get_remain(), "%s:%s\r\n",
                         s.first.c_str(), s.second.c_str());
            // we assume it will success
        }
        pos += n;
        buf->set_last(pos);
    }

    if (buf->get_remain() < 2) {
        chain.emplace_back(2);
        buf = &chain.back();
        pos = buf->get_pos();
    }

    pos[0] = '\r';
    pos[1] = '\n';

    buf->set_last(pos + 2);

    int rc = conn->send_chain(chain);

    if (rc == SERVX_ERROR) {
        return SERVX_ERROR;
    }

    return rc == SERVX_OK ? SERVX_OK : SERVX_AGAIN;
}

int HttpResponse::send_body(std::unique_ptr<File>&& p) {
    std::list<std::unique_ptr<File>> &files = out.back().files;
    files.emplace_back(std::move(p));
    return send();
}

int HttpResponse::send_body(std::list<Buffer>&& chain) {
    if (!out.back().files.empty()) {
        out.emplace_back();
    }
    auto &l = out.back().chain;
    l.splice(l.end(), std::move(chain));
    return send();
}

int HttpResponse::send() {
    int rc = SERVX_OK;

    while (!out.empty()) {
        Sendable &sa = out.front();

        if (!sa.chain.empty()) {
            rc = conn->send_chain(sa.chain);
            if (rc == SERVX_ERROR) {
                return SERVX_ERROR;
            }
            if (rc != SERVX_OK) {
                return SERVX_AGAIN;
            }
        }

        if (!sa.files.empty()) {
            if (location->is_send_file()) {
                for (auto &f : sa.files) {
                    rc = conn->send_file(f.get());
                    if (rc == SERVX_ERROR) {
                        return SERVX_ERROR;
                    }
                    if (rc != SERVX_OK) {
                        return SERVX_AGAIN;
                    }
                }
            } else {
                for (auto &f : sa.files) {
                    if (!f->file_status()) {
                        return SERVX_ERROR;
                    }
                    sa.chain.emplace_back(f->get_file_size());
                    // TODO: the result of read
                    f->read(sa.chain.back().get_pos(), f->get_file_size());
                }
                continue;
            }
        }

        out.pop_front();
    }

    return SERVX_OK;
}

}
