#ifndef _MODULE_MANAGER_
#define _MODULE_MANAGER_

#include "http_module.h"

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

private:
    Module* modules[NULL_MODULE];
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

class HttpModuleManager {
public:
    template <typename T>
    typename T::main_conf_t* get_main_conf() const;

    template <typename T>
    typename T::main_conf_t* get_srv_conf(Server& srv) const;

    template <typename T>
    typename T::main_conf_t* get_loc_conf(Location& loc) const;

    HttpModule* get_module(int index) const;

    static HttpModuleManager* instance() { return manager; }

private:
    HttpModuleManager() = default;

private:
    static HttpModuleManager* manager;
};

template <typename T>
inline typename T::main_conf_t* HttpModuleManager::get_main_conf() const {
    return ModuleManager::instance()->get_conf<T::main_conf_t>();
}

template <typename T>
inline typename T::main_conf_t* HttpModuleManager::get_srv_conf(
    Server& srv) const {
    return static_cast<typename T::srv_conf_t*>(srv.get_conf(T::index));
}

template <typename T>
inline typename T::main_conf_t* HttpModuleManager::get_loc_conf(
    Location& loc) const {
    return static_cast<typename T::srv_conf_t*>(loc.get_conf(T::index));
}

inline HttpModule* HttpModuleManager::get_module(int index) const {
    return reinterpret_cast<HttpModule*>(
            ModuleManager::instance()->get_module(index));
}


}

#endif
