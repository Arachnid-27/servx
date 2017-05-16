#ifndef _CONNECTION_POLL_
#define _CONNECTION_POLL_

#include <unordered_set>
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

    Connection* get_connection(int fd, bool lst = false);
    void ret_connection(Connection* conn);

    void enable_reusable(Connection* conn);
    void disable_reusable(Connection* conn);

    static ConnectionPool* instance() { return pool; }

private:
    ConnectionPool(): pool_start(nullptr) {}

private:
    Connection *pool_start;
    std::vector<Connection*> free_connections;
    std::unordered_set<Connection*> reusable_connections;

    static ConnectionPool *pool;
};

inline void ConnectionPool::enable_reusable(Connection* conn) {
    reusable_connections.insert(conn);
}

inline void ConnectionPool::disable_reusable(Connection* conn) {
    reusable_connections.erase(conn);
}

}

#endif
