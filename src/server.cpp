#include "server.h"

#include "module_manager.h"

namespace servx {

Server::Server() {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        confs[i] = manager->get_module(i)->create_srv_conf();
    }
}

bool Server::push_location(Location* loc) {
    if (loc->is_regex()) {
        regex_locations.push_back(loc);
        return true;
    }

    return prefix_locations.push(loc);
}

Location* Server::serach(const std::string& uri) {
    // Todo regex_locations

    return prefix_locations.search(uri);
}

}
