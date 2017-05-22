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
    int rc;

    while (!out.empty()) {
        auto &chain = out.front().chain;

        if (!chain.empty()) {
            rc = conn->send_chain(chain);
            if (rc == SERVX_ERROR) {
                return SERVX_ERROR;
            }
            if (rc != SERVX_OK) {
                return SERVX_AGAIN;
            }
        }

        auto &files = out.front().files;

        if (!files.empty()) {
            if (location->is_send_file()) {
                auto iter = files.begin();
                while (!files.empty()) {
                    rc = conn->send_file(iter->get());
                    if (rc == SERVX_ERROR) {
                        return SERVX_ERROR;
                    }
                    if (rc != SERVX_OK) {
                        return SERVX_AGAIN;
                    }
                    iter = files.erase(iter);
                }
            } else {
                chain.emplace_back(1024);
                auto iter = files.begin();
                while (!files.empty()) {
                    if (!(*iter)->file_status()) {
                        return SERVX_ERROR;
                    }

                    rc = io_read((*iter)->get_fd(), chain.back().get_last(),
                        chain.back().get_remain());

                    if (rc < 0) {
                        return rc;
                    }

                    int bytes = (*iter)->get_offset() + rc;
                    (*iter)->set_offset(bytes);
                    chain.back().set_last(chain.back().get_pos() + rc);

                    if (bytes == (*iter)->get_file_size()) {
                        iter = files.erase(iter);
                        continue;
                    } else if (chain.back().get_remain() == 0) {
                        chain.emplace_back(1024);
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

}
