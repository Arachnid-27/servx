#include "server.h"

#include "module_manager.h"
#include "http_module.h"
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

Buffer* Server::get_buffer() {
    if (free_bufs.empty()) {
        int size = get_core_conf()->client_buffer_size;
        all_bufs.emplace_back(size);
        return &all_bufs.back();
    }

    Buffer *buf = free_bufs.back();
    free_bufs.pop_back();

    Logger::instance()->debug("[http server %p] get buf %p, total %d, left %d",
        this, buf, all_bufs.size(), free_bufs.size());

    return buf;
}

void Server::ret_buffer(Buffer* buf) {
    buf->reset();
    free_bufs.push_back(buf);

    Logger::instance()->debug("[http server %p] ret buf %p, total %d, left %d",
        this, buf, all_bufs.size(), free_bufs.size());
}

}
