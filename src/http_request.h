#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "listener.h"

namespace servx {

class HttpRequest {
};

class HttpConnection {
public:
    void set_listening(std::shared_ptr<Listening> listening);

private:
    std::shared_ptr<Listening> listening;
};

void http_wait_request_handler(Event* ev);

void http_empty_handler(Event* ev);

}

#endif
