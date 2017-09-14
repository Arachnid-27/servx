#ifndef _HTTP_UPSTREAM_RESPONSE_
#define _HTTP_UPSTREAM_RESPONSE_

#include <functional>

#include "buffer.h"
#include "http_request.h"

namespace servx {

class HttpUpstreamRequest;

class HttpUpstreamResponse {
public:
    HttpUpstreamResponse(HttpUpstreamRequest* r,
        std::function<int(HttpRequest*, Buffer*)> rhh,
        std::function<int(HttpRequest*, Buffer*)> rbh)
        : req(r), content_length(-1), recv(0),
          response_header_buf(nullptr),
          response_header_handler(rhh),
          response_body_handler(rbh) {}

    HttpResponseHeader* get_header() { return header.get(); }

    void recv_response_line_handler(Event* ev);
    void recv_response_headers_handler(Event* ev);
    void recv_response_body_handler(Event* ev);

private:
    bool check_header_buf_full();
    void recv_response_headers(Event* ev);
    void response_header_done(Event* ev);
    int handle_response_body();
    int read_response_header(Connection* conn);

    HttpUpstreamRequest *req;

    std::unique_ptr<HttpResponseHeader> header;

    long content_length;
    long recv;

    Buffer *response_header_buf;
    std::list<Buffer*> response_body_bufs;

    std::function<int(HttpRequest*, Buffer*)> response_header_handler;
    std::function<int(HttpRequest*, Buffer*)> response_body_handler;
};

}

#endif
