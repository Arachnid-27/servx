#ifndef _SERVER_H_
#define _SERVER_H_

#include <unordered_set>
#include <vector>

#include "listener.h"
#include "location.h"
#include "module.h"

namespace servx {

struct HttpCoreSrvConf: public ModuleConf {
    int client_header_timeout;
    int client_body_timeout;
    int client_header_buffer_size;
    int client_body_buffer_size;
};

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

    Buffer* get_body_buf();

    void ret_body_buf(Buffer* buf) { free_body_bufs.push_back(buf); }

    template <typename T>
    typename T::srv_conf_t* get_conf();

    HttpCoreSrvConf* get_core_conf();

private:
    std::unordered_set<std::string> server_names;
    std::vector<std::unique_ptr<Location>> regex_locations;
    LocationTree prefix_locations;
    std::vector<Buffer> all_bufs;
    std::vector<Buffer*> free_body_bufs;
    std::unique_ptr<ModuleConf> confs[NULL_MODULE];
};

inline bool Server::contain_server_name(const std::string& name) const {
    return server_names.find(name) != server_names.end();
}

template <typename T>
inline typename T::srv_conf_t* Server::get_conf() {
    return static_cast<typename T::srv_conf_t*>(confs[T::index].get());
}

inline HttpCoreSrvConf* Server::get_core_conf() {
    return static_cast<HttpCoreSrvConf*>(confs[HTTP_CORE_MODULE].get());
}

}

#endif
