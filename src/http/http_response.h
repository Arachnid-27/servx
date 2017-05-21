#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <list>
#include <memory>
#include <unordered_map>

#include "file.h"
#include "http.h"
#include "location.h"

namespace servx {

class HttpResponse {
public:
    explicit HttpResponse(Connection* c);

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

    int send_header();
    int send_body(std::unique_ptr<File>&& p);
    int send_body(std::list<Buffer>&& chain);
    int send();

    void set_etag(bool e) { etag = e; };

    void set_location(Location* loc) { location = loc; }

private:
    struct Sendable {
        std::list<Buffer> chain;
        std::list<std::unique_ptr<File>> files;
    };

    std::unordered_map<std::string, std::string> headers;
    Connection *conn;
    Location *location;
    long content_length;
    long last_modified_time;
    int status;

    uint32_t header_only:1;
    uint32_t chunked:1;
    uint32_t keep_alive:1;
    uint32_t etag:1;

    std::list<Sendable> out;

    static std::string status_lines[HTTP_GATEWAY_TIME_OUT + 1];
};

template <typename T1, typename T2>
inline void HttpResponse::set_headers(T1&& name, T2&& value) {
    headers.emplace(std::forward<T1>(name), std::forward<T2>(value));
}

}

#endif
