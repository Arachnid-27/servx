#include "connection_pool.h"

#include "logger.h"

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
    Connection *conn = nullptr;

    if (!free_connections.empty()) {
        conn = free_connections.back();
        free_connections.pop_back();
    } else if (!reusable_connections.empty()) {
        Logger::instance()->warn("reusing connection");
        auto iter = reusable_connections.begin();
        conn = *iter;
        conn->close();
        reusable_connections.erase(iter);
    }

    if (conn != nullptr) {
        conn->open(fd, lst);
    }

    return conn;
}

void ConnectionPool::ret_connection(Connection* conn) {
    disable_reusable(conn);
    free_connections.push_back(conn);
}

}
