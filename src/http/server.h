#ifndef _SERVER_H_
#define _SERVER_H_

#include <unordered_set>
#include <vector>

#include "http_location_tree.h"
#include "listener.h"
#include "module.h"

namespace servx {

class Server {
public:
    Server();

    Server(const Server&) = delete;
    Server(const Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;

    ~Server() = default;

    void push_server_name(const std::string& s) { server_names.insert(s); }

    bool push_location(Location* loc, bool regex);

    bool contain_server_name(const std::string& name) const;

    Location* find_location(const std::string& uri);

    int get_timeout() const { return timeout; }
    void set_timeout(int t) { timeout = t; }

    int get_buffer_size() { return buffer_size; }
    void set_buffer_size(int sz) { buffer_size = sz; }

    template <typename T>
    typename T::srv_conf_t* get_conf();

private:
    int timeout;
    int buffer_size;

    std::unordered_set<std::string> server_names;
    std::vector<std::unique_ptr<Location>> regex_locations;
    LocationTree prefix_locations;
    std::unordered_map<uintptr_t, std::unique_ptr<HttpConf>> confs;
};

inline bool Server::contain_server_name(const std::string& name) const {
    return server_names.find(name) != server_names.end();
}

template <typename T>
inline typename T::srv_conf_t* Server::get_conf() {
    return static_cast<typename T::srv_conf_t*>(
        confs[reinterpret_cast<uintptr_t>(&T::instance)].get());
}

}

#endif
