#include "connection_pool.h"

namespace servx {

ConnectionPool::~ConnectionPool() {
    delete[] pool_start;
}

ConnectionPool* ConnectionPool::pool = new ConnectionPool;

void ConnectionPool::init(size_t sz) {
    if (pool_start) {
        // err_log
        return;
    }

    pool_start = new Connection[sz];
    free_connections = std::vector<Connection*>(sz);
    for (size_t i = 0; i < sz; ++i) {
        free_connections[i] = pool_start + i;
    }
}

Connection* ConnectionPool::get_connection(int fd, bool lst) {
    auto conn = free_connections.back();
    free_connections.pop_back();
    conn->open(fd, lst);
    return conn;
}

void ConnectionPool::ret_connection(Connection* conn) {
    free_connections.push_back(conn);
}

}
