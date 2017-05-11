#include "server.h"

#include "module_manager.h"

namespace servx {

Server::Server() {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        confs[i] = std::unique_ptr<ModuleConf>(
            manager->get_module(i)->create_srv_conf());
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

}
