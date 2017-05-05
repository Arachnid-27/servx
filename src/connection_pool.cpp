#include "connection_pool.h"

namespace servx {

void ConnectionDeleter::operator()(Connection* conn) {
    conn->close();
    ConnectionPool::pool->ret_connection(conn);
}

ConnectionPool* ConnectionPool::pool = new ConnectionPool;

void ConnectionPool::init(int size) {
    Connection *conns = new Connection[size];
    std::deque<Connection*> temp(size);

    for (auto& c : temp) {
        c = (conns++);
    }
    free_connections = std::queue<Connection*>(temp);
}

Connection* ConnectionPool::get_connection(int fd) {
    Connection *conn = free_connections.front();
    auto ptr = std::shared_ptr<Connection>(conn, ConnectionDeleter());
    conn->open(fd);
    free_connections.pop();
    return conn;
}

void ConnectionPool::ret_connection(Connection* conn) {
    free_connections.push(conn);
}

}
