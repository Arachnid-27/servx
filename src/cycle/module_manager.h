#ifndef _MODULE_MANAGER_
#define _MODULE_MANAGER_

#include "module.h"

namespace servx {

class ModuleManager {
public:
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;

    Command* find_command(const std::string& parent_name,
        const std::string& name) const;

    bool for_each(std::function<bool (Module*)> func);

    bool for_each_http(std::function<bool (HttpModule*)> func);

    static ModuleManager* instance() { return manager; }

private:
    ModuleManager();

    template <typename T, typename U>
    void install_module(T* module, std::vector<U*>& vec);

    std::vector<CoreModule*> core_modules;
    std::vector<EventModule*> event_modules;
    std::vector<HttpModule*> http_modules;

    std::unordered_map<std::string,
        std::unordered_map<std::string, Command*>> commands;

    static ModuleManager* manager;
};

}

#endif
