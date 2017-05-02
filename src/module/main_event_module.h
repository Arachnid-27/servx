#ifndef _CORE_EVENT_MODULE_H_
#define _CORE_EVENT_MODULE_H_

#include "core.h"

struct MainEventModuleConf {
    unsigned int connections;
    std::string use;
};

class MainEventModule:
    public EventModule<MainEventModuleConf, ModuleIndex::MAIN_EVENT_MODULE> {
public:
    MainEventModule(): EventModule({
                    new Command("connections", connections_handler),
                    new Command("use", use_handler),
                   },
                   new MainEventModuleConf) {}

public:
    static bool connections_handler(const std::vector<std::string>&);

    static bool use_handler(const std::vector<std::string>&);
};

#endif
