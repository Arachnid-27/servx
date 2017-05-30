#ifndef _HTTP_CONNECTION_
#define _HTTP_CONNECTION_

#include "http_listening.h"
#include "http_request.h"

#include "logger.h"

namespace servx {

class HttpConnection: public ConnectionContext {
public:
    HttpConnection(HttpListening* lst): listening(lst) {}

    HttpConnection(const HttpConnection&);
    HttpConnection(HttpConnection&&);
    HttpConnection& operator=(const HttpConnection&);
    HttpConnection& operator=(HttpConnection&&);

    ~HttpConnection() override = default;

    HttpListening* get_listening() const { return listening; }

    HttpRequest* get_request() const { return request.get(); }
    void close_request() { request = nullptr; }

    void wait_request(Event* ev);

private:
    HttpListening *listening;
    std::unique_ptr<HttpRequest> request;
};

}

#endif
