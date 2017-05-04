#ifndef _MODULE_MANAGER_
#define _MODULE_MANAGER_

#include "module.h"

namespace servx {

class ModuleManager {
public:
    Command* find_command(const std::string& name) const;
    
    template <class T>
    decltype(((T*) nullptr)->get_conf()) get_conf() const;

    bool for_each(std::function<bool (Module*)> func);

public:
    static ModuleManager* instance() { return manager; }

private:
    ModuleManager();

    void create_module(ModuleIndex index, Module* module);

private:
    Module* modules[ModuleIndex::NULL_MODULE];
    std::unordered_map<std::string, Command*> commands;

private:
    static ModuleManager* manager;
};

template <class T>
inline decltype(((T*) nullptr)->get_conf()) ModuleManager::get_conf() const {
    return static_cast<T*>(modules[T::get_index()])->get_conf();
}

}

#endif
