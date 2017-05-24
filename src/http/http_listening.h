#ifndef _HTTP_LISTENING_
#define _HTTP_LISTENING_

#include "listener.h"
#include "server.h"

namespace servx {

class HttpListening: public ListeningContext {
public:
    HttpListening(): default_server(nullptr) {}

    HttpListening(const Server&) = delete;
    HttpListening(Server&&) = delete;
    HttpListening& operator=(const Server&) = delete;
    HttpListening& operator=(Server&&) = delete;

    ~HttpListening() = default;

    bool push_server(Server* srv, bool def);

    Server* search_server(const std::string& name);

    Server* get_default_server() { return default_server; }

    static void init_connection(Connection* conn);

private:
    Server* default_server;
    std::vector<Server*> servers;
};

}

#endif
