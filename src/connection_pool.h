#ifndef _CONNECTION_POLL_
#define _CONNECTION_POLL_

#include <vector>

#include "connection.h"

namespace servx {

class ConnectionPool {
public:
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool(ConnectionPool&&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    ConnectionPool& operator=(ConnectionPool&&) = delete;

    ~ConnectionPool();

    void init(size_t sz);

    Connection* get_connection(int fd, bool lst);

    void ret_connection(Connection* conn);

    static ConnectionPool* instance() { return pool; }

private:
    ConnectionPool(): pool_start(nullptr) {}

private:
    Connection *pool_start;
    std::vector<Connection*> free_connections;

    static ConnectionPool *pool;
};

}

#endif
