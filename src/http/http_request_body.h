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
    int discard();

    long get_content_length() const { return content_length; }
    void set_content_length(long n) { content_length = n; }

    static void read_request_body_handler(HttpRequest* req);
    static void discard_request_body_handler(HttpRequest* req);

private:
    int handle_read();
    int handle_discard();

    HttpRequest *req;
    std::list<Buffer> body;
    http_req_handler_t handler;

    std::unique_ptr<Buffer> discard_buffer;

    uint32_t discarded:1;

    long content_length;
    long recv;
};

}

#endif
