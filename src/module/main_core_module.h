#ifndef _MAIN_CORE_MODULE_
#define _MAIN_CORE_MODULE_

#include "core.h"

struct MainCoreModuleConf {
    int worker;
    bool daemon;
};

class MainCoreModule:
    public CoreModule<MainCoreModuleConf, ModuleIndex::MAIN_CORE_MODULE> {
public:
    MainCoreModule(): CoreModule({
                    new Command("worker", worker_handler, 1),
                    new Command("daemon", daemon_handler, 1),
                    new Command("error_log", error_log_handler)
                   },
                   new MainCoreModuleConf) {}

    virtual bool init_conf();

public:
    static bool worker_handler(const std::vector<std::string>& v);

    static bool daemon_handler(const std::vector<std::string>& v);

    static bool error_log_handler(const std::vector<std::string>& v);
};

#endif
