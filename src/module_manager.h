#ifndef _MODULE_MANAGER_
#define _MODULE_MANAGER_

#include "http_module.h"

namespace servx {

class HttpModuleManager;

class ModuleManager {
    friend class HttpModuleManager;

public:
    ModuleManager(const ModuleManager&) = delete;

    Command* find_command(const std::string& name) const;

    template <typename T>
    typename T::conf_t get_conf() const;

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

template <typename T>
inline typename T::conf_t ModuleManager::get_conf() const {
    return static_cast<T*>(modules[T::index])->get_conf();
}

class HttpModuleManager {
public:
    template <typename T>
    typename T::main_conf_t get_main_conf() const;

    template <typename T>
    typename T::main_conf_t get_srv_conf(Server& srv) const;

    template <typename T>
    typename T::main_conf_t get_loc_conf(Location& loc) const;

    HttpModule* get_module(int index) const {
        return reinterpret_cast<HttpModule*>(
                ModuleManager::manager->modules[index]);
    }

    static HttpModuleManager* instance() { return manager; }

private:
    HttpModuleManager();

    void create_module(int index, HttpModule* module);

private:
    HttpModuleConf* main_confs[NULL_MODULE];

    static HttpModuleManager* manager;
};

template <typename T>
typename T::main_conf_t HttpModuleManager::get_main_conf() const {
    return static_cast<typename T::main_conf_t>(main_confs[T::index]);
}

template <typename T>
typename T::main_conf_t HttpModuleManager::get_srv_conf(Server& srv) const {
    return static_cast<typename T::srv_conf_t>(srv.get_conf(T::index));
}

template <typename T>
typename T::main_conf_t HttpModuleManager::get_loc_conf(Location& loc) const {
    return static_cast<typename T::srv_conf_t>(loc.get_conf(T::index));
}

}

#endif
