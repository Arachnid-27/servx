#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>

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

    void push_server_name(const std::string& s) { server_names.push_back(std::move(s)); }

    bool push_location(const std::string& uri, bool regex);

    std::shared_ptr<Location> serach(const std::string& uri);

    ModuleConf* get_conf(int index) { return confs[index].get(); }

private:
    std::vector<std::string> server_names;
    std::vector<std::shared_ptr<Location>> regex_locations;
    LocationTree prefix_locations;
    std::unique_ptr<ModuleConf> confs[NULL_MODULE];
};

}

#endif
