#ifndef _MODULE_MANAGER_
#define _MODULE_MANAGER_

#include "module.h"
#include "modules.h"

namespace servx {

class ModuleManager {
public:
    ModuleManager(const ModuleManager&) = delete;

    Command* find_command(const std::string& name) const;

    template <typename T>
    typename T::conf_t* get_conf() const;

    Module* get_module(int index) const;

    bool for_each(std::function<bool (Module*)> func);

    static ModuleManager* instance() { return manager; }

private:
    ModuleManager();

    void create_module(int index, Module* module);

    Module* modules[NULL_MODULE];
    // TODO: use double map
    std::unordered_map<std::string, Command*> commands;

    static ModuleManager* manager;
};

inline Module* ModuleManager::get_module(int index) const {
    return reinterpret_cast<Module*>(modules[index]);
}

template <typename T>
inline typename T::conf_t* ModuleManager::get_conf() const {
    return static_cast<T*>(modules[T::index])->get_conf();
}

}

#endif
