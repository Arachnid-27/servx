#include "module_manager.h"

#include "core_module.h"
#include "epoll_module.h"
#include "event_module.h"
#include "http_module.h"
#include "http_static_module.h"

namespace servx {

ModuleManager* ModuleManager::manager = new ModuleManager;

ModuleManager::ModuleManager() {
    create_module(MainCoreModule::index, new MainCoreModule);
    create_module(MainEventModule::index, new MainEventModule);
    create_module(MainHttpModule::index, new MainHttpModule);
    create_module(EpollModule::index, new EpollModule);
    create_module(HttpStaticModule::index, new HttpStaticModule);
}

void ModuleManager::create_module(int index, Module* module) {
    modules[index] = module;
    for (auto c : module->get_commands()) {
        // the lastest command will replace old command if have same name
        commands[c->get_name()] = c;
    }
}

Command* ModuleManager::find_command(const std::string& name) const {
    auto it = commands.find(name);
    if (it == commands.end()) {
        return nullptr;
    }
    return it->second;
}

bool ModuleManager::for_each(std::function<bool (Module*)> func) {
    for (size_t i = 0; i < ModuleIndex::NULL_MODULE; ++i) {
        if (!func(modules[i])) {
            return false;
        }
    }

    return true;
}

HttpModuleManager* HttpModuleManager::manager = new HttpModuleManager;

}
