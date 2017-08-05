#include "server.h"

#include "module_manager.h"
#include "http_module.h"
#include "logger.h"

namespace servx {

Server::Server(): timeout(60000), buffer_size(4096) {
    auto manager = ModuleManager::instance();
    manager->for_each_http([this](HttpModule* module) {
            confs[*reinterpret_cast<uintptr_t*>(&module)] =
                std::unique_ptr<HttpConf>(module->create_srv_conf());
            return true;
        });
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
        all_bufs.emplace_back(buffer_size);
        return &all_bufs.back();
    }

    Buffer *buf = free_bufs.back();
    free_bufs.pop_back();
    return buf;
}

void Server::ret_buffer(Buffer* buf) {
    buf->reset();
    free_bufs.push_back(buf);
}

}
