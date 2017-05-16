#include "server.h"

#include <algorithm>

#include "module_manager.h"

namespace servx {

Server::Server() {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        auto module = manager->get_module(i);
        if (module != nullptr) {
            confs[i] = std::unique_ptr<ModuleConf>(module->create_srv_conf());
        }
    }
}

bool Server::push_location(const std::string& uri, bool regex) {
    if (regex) {
        regex_locations.emplace_back(std::make_shared<Location>(uri));
        return true;
    }

    return prefix_locations.push(uri);
}

std::shared_ptr<Location> Server::serach(const std::string& uri) {
    // Todo regex_locations

    return prefix_locations.search(uri);
}

bool HttpServers::push_server(Server* srv, bool def) {
    if (def) {
        if (default_server != nullptr) {
            return false;
        }
        default_server = srv;
    }
    servers.push_back(srv);

    return true;
}

Server* HttpServers::search_server(const std::string& name) {
    auto iter = std::find_if(servers.begin(), servers.end(),
        [&](const Server* srv) { return srv->contain_server_name(name); });

    if (iter != servers.end()) {
        return *iter;
    }
    return default_server;
}

}
