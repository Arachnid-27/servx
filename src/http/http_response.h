#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

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

    bool send_header();

    bool set_etag();

private:
    std::unique_ptr<Buffer> send_buf;
    std::unordered_map<std::string, std::string> headers;
    long last_modified_time;
    long content_length;
    int status;

    static std::unordered_map<int, std::string> status_lines;
};

template <typename T1, typename T2>
inline void HttpResponse::set_headers(T1&& name, T2&& value) {
    headers.emplace(std::forward<T1>(name), std::forward<T2>(value));
}

}

#endif
