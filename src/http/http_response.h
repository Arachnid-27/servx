#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <list>
#include <memory>
#include <unordered_map>

#include "buffer.h"

namespace servx {

enum HttpStateCode {
    HTTP_CONTINUE = 100,

    HTTP_OK = 200,
    HTTP_PARTIAL_CONTENT = 206,

    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_MOVED_TEMPORARILY = 302,
    HTTP_NOT_MODIFIED = 304,

    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_NOT_ALLOWED = 405,
    HTTP_REQUEST_TIME_OUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_REQUEST_URI_TOO_LARGE = 414,

    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIME_OUT = 504
};

class HttpResponse {
public:
    HttpResponse();

    HttpResponse(const HttpResponse&) = delete;
    HttpResponse(HttpResponse&&) = delete;
    HttpResponse& operator=(const HttpResponse&) = delete;
    HttpResponse& operator=(HttpResponse&&) = delete;

    ~HttpResponse() = default;

    template <typename T1, typename T2>
    void set_headers(T1&& name, T2&& value);

    long get_content_length() const { return content_length; }
    void set_content_length(long n) { content_length = n; }

    long get_last_modified_time() const { return last_modified_time; }
    void set_last_modified_time(long t) { last_modified_time = t; }

    int get_status() const { return status; }
    void set_status(int s) { status = s; }

    bool is_header_only() const { return header_only; }
    void set_header_only(bool h) { header_only = h; }

    bool is_chunked() const { return chunked; }
    void set_chunked(bool c) { chunked = c; }

    bool is_keep_alive() const { return keep_alive; }
    void set_keep_alive(bool k) { keep_alive = k; }

    bool send_header();

    void set_etag(bool e) { etag = e; };

private:
    std::unordered_map<std::string, std::string> headers;
    long content_length;
    long last_modified_time;
    int status;

    uint32_t header_only:1;
    uint32_t chunked:1;
    uint32_t keep_alive:1;
    uint32_t etag:1;

    std::list<Buffer> out;

    static std::unordered_map<int, std::string> status_lines;
};

template <typename T1, typename T2>
inline void HttpResponse::set_headers(T1&& name, T2&& value) {
    headers.emplace(std::forward<T1>(name), std::forward<T2>(value));
}

}

#endif
