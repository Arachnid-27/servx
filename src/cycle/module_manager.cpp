#include "module_manager.h"

#include "core_module.h"
#include "epoll_module.h"
#include "event_module.h"
#include "http_core_module.h"
#include "http_static_module.h"
#include "http_upstream_module.h"
#include "http_proxy_module.h"

namespace servx {

ModuleManager* ModuleManager::manager = new ModuleManager;

ModuleManager::ModuleManager() {
    create_module(MainCoreModule::index, new MainCoreModule);
    create_module(MainEventModule::index, new MainEventModule);
    create_module(EpollModule::index, new EpollModule);
    create_module(HttpCoreModule::index, new HttpCoreModule);
    create_module(HttpStaticModule::index, new HttpStaticModule);
    create_module(HttpUpstreamModule::index, new HttpUpstreamModule);
    create_module(HttpProxyModule::index, new HttpProxyModule);
}

void ModuleManager::create_module(int index, Module* module) {
    modules[index] = module;
    for (auto c : module->get_commands()) {
        commands[c->get_block_context()][c->get_name()] = c;
    }
}

Command* ModuleManager::find_command(int type, const std::string& name) const {
    auto it = commands[type].find(name);
    if (it == commands[type].end()) {
        return nullptr;
    }
    return it->second;
}

bool ModuleManager::for_each(std::function<bool (Module*)> func) {
    for (size_t i = 0; i < NULL_MODULE; ++i) {
        if (!func(modules[i])) {
            return false;
        }
    }

    return true;
}

}
