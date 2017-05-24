#include "server.h"

#include "module_manager.h"
#include "logger.h"
#include "http_module.h"

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

}
