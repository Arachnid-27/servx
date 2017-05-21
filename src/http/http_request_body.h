#ifndef _HTTP_REQUEST_BODY_
#define _HTTP_REQUEST_BODY_

#include <list>

#include "buffer.h"
#include "http_request.h"

namespace servx {

class HttpRequest;

class HttpRequestBody {
public:
    using http_req_handler_t = std::function<void(HttpRequest*)>;

    HttpRequestBody(HttpRequest* r): req(r), content_length(-1), recv(0) {}

    HttpRequestBody(const HttpRequestBody&) = delete;
    HttpRequestBody(HttpRequestBody&&) = delete;
    HttpRequestBody& operator=(const HttpRequestBody&) = delete;
    HttpRequestBody& operator=(HttpRequestBody&&) = delete;

    ~HttpRequestBody() = default;

    int read(const http_req_handler_t& h);
    int read();
    int discard();

    long get_content_length() const { return content_length; }
    void set_content_length(long n) { content_length = n; }

    bool is_chunked() const { return chunked; }
    void set_chunked(bool c) { chunked = c; }

    static void read_request_body_handler(HttpRequest* req);

private:
    HttpRequest *req;
    std::list<Buffer> body;
    http_req_handler_t handler;

    uint32_t discarded:1;
    uint32_t chunked:1;

    long content_length;
    long recv;
};

}

#endif
