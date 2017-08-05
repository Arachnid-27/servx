#include "module_manager.h"

#include "core_module.h"
#include "epoll_module.h"
#include "event_module.h"
#include "http_module.h"
#include "http_proxy_module.h"
#include "http_static_module.h"
#include "http_upstream_module.h"

namespace servx {

ModuleManager* ModuleManager::manager = new ModuleManager;

ModuleManager::ModuleManager() {
    install_module(&MainCoreModule::instance, core_modules);
    install_module(&EventCoreModule::instance, core_modules);
    install_module(&EpollModule::instance, event_modules);
    install_module(&HttpMainModule::instance, http_modules);
    install_module(&HttpStaticModule::instance, http_modules);
    install_module(&HttpUpstreamModule::instance, http_modules);
    install_module(&HttpProxyModule::instance, http_modules);
}

template <typename T, typename U>
void ModuleManager::install_module(T* module, std::vector<U*>& vec) {
    vec.push_back(static_cast<U*>(module));
    for (auto &c : T::commands) {
        commands[c->get_parent_name()][c->get_name()] = std::move(c);
    }
}

Command* ModuleManager::find_command(const std::string& parent_name,
        const std::string& name) const {
    auto it1 = commands.find(parent_name);
    if (it1 != commands.end()) {
        auto it2 = it1->second.find(name);
        if (it2 != it1->second.end()) {
            return it2->second;
        }
    }
    return nullptr;
}

bool ModuleManager::for_each(std::function<bool (Module*)> func) {
    for (auto m : core_modules) {
        if (!func(m)) {
            return false;
        }
    }
    for (auto m : event_modules) {
        if (!func(m)) {
            return false;
        }
    }
    for (auto m : http_modules) {
        if (!func(m)) {
            return false;
        }
    }
    return true;
}

bool ModuleManager::for_each_http(std::function<bool (HttpModule*)> func) {
    for (auto m : http_modules) {
        if (!func(m)) {
            return false;
        }
    }
    return true;
}

}
