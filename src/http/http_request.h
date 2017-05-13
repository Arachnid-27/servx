#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "server.h"

namespace servx {

class HttpRequest {
};

class HttpConnection: public ConnectionContext {
public:
    void set_server(Server* srv) { server = srv; }

private:
    Server *server;
};

void http_wait_request_handler(Event* ev);

void http_empty_handler(Event* ev);

void http_init_connection(Connection* conn);

}

#endif
