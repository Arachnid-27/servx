#ifndef _SERVER_H_
#define _SERVER_H_

#include <unordered_set>
#include <vector>

#include "listener.h"
#include "location.h"

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

    bool push_location(const std::string& uri, bool regex);

    bool contain_server_name(const std::string& name) const;

    std::shared_ptr<Location> serach(const std::string& uri);

    ModuleConf* get_conf(int index) { return confs[index].get(); }

private:
    std::unordered_set<std::string> server_names;
    std::vector<std::shared_ptr<Location>> regex_locations;
    LocationTree prefix_locations;
    std::unique_ptr<ModuleConf> confs[NULL_MODULE];
};

inline bool Server::contain_server_name(const std::string& name) const {
    return server_names.find(name) != server_names.end();
}

class HttpServers: public ListeningServers {
public:
    HttpServers() = default;

    HttpServers(const Server&) = delete;
    HttpServers(Server&&) = delete;
    HttpServers& operator=(const Server&) = delete;
    HttpServers& operator=(Server&&) = delete;

    ~HttpServers() = default;

    bool push_server(Server* srv, bool def);

    Server* search_server(const std::string& name);

    Server* get_default_server() { return default_server; }

private:
    Server* default_server;
    std::vector<Server*> servers;
};

}

#endif
