#include "module_manager.h"

#include "core_module.h"
#include "epoll_module.h"
#include "event_module.h"

namespace servx {

ModuleManager* ModuleManager::manager = new ModuleManager;

ModuleManager::ModuleManager() {
    create_module(ModuleIndex::MAIN_CORE_MODULE, new MainCoreModule);
    create_module(ModuleIndex::MAIN_EVENT_MODULE, new MainEventModule);
    create_module(ModuleIndex::EPOLL_MODULE, new EpollModule);
}

void ModuleManager::create_module(ModuleIndex index, Module* module) {
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

}
