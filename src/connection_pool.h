#ifndef _CONNECTION_POLL_
#define _CONNECTION_POLL_

#include <memory>
#include <queue>

#include "connection.h"

namespace servx {

class ConnectionDeleter {
public:
    void operator()(Connection* conn);
};

class ConnectionPool {
    friend class ConnectionDeleter;

public:
    ~ConnectionPool();

    void init(int size);

    Connection* get_connection(int fd);

    void ret_connection(Connection* conn);

    static ConnectionPool* instance() { return pool; }

private:
    ConnectionPool() {}

private:
    std::queue<Connection*> free_connections;

    static ConnectionPool *pool;
};

}

#endif
