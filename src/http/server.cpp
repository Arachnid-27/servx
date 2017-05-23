#include "server.h"

#include <algorithm>

#include "http_module.h"
#include "module_manager.h"
#include "logger.h"

namespace servx {

Server::Server() {
    auto manager = ModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        auto module = manager->get_module(i);
        if (module != nullptr && module->get_type()== HTTP_MODULE) {
            confs[i] = std::unique_ptr<ModuleConf>(
                static_cast<HttpModule*>(module)->create_srv_conf());
        }
    }
}

bool Server::push_location(Location* loc, bool regex) {
    if (regex) {
        regex_locations.emplace_back(loc);
        return true;
    }

    return prefix_locations.push(loc);
}

Location* Server::find_location(const std::string& uri) {
    // TODO: regex_locations

    return prefix_locations.find(uri);
}

bool HttpServers::push_server(Server* srv, bool def) {
    if (def) {
        if (default_server != nullptr) {
            Logger::instance()->error("default server exists!");
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
