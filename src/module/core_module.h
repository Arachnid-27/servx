#ifndef _CORE_MODULE_
#define _CORE_MODULE_

#include "module.h"

namespace servx {

class CoreModule: public Module {
protected:
    CoreModule(const std::initializer_list<Command*>& v): Module(CORE_MODULE, v) {}
};

struct CoreModuleConf {
    int rlimit_nofile;
    int worker;
    bool daemon;
};

class MainCoreModule: public ModuleWithConf<CoreModule,
                                            CoreModuleConf,
                                            ModuleIndex::MAIN_CORE_MODULE> {
public:
    MainCoreModule(): ModuleWithConf(
        {
            new Command(ModuleType::CORE_MODULE, "worker",
                        worker_handler, 1),
            new Command(ModuleType::CORE_MODULE, "daemon",
                        daemon_handler, 1),
            new Command(ModuleType::CORE_MODULE, "rlimit_nofile",
                        rlimit_nofile_handler, 1),
            new Command(ModuleType::CORE_MODULE, "error_log",
                        error_log_handler)
        }) {}

    bool init_conf() override;

    bool init_module() override;

public:
    static bool worker_handler(command_vals_t v);

    static bool daemon_handler(command_vals_t v);

    static bool rlimit_nofile_handler(command_vals_t v);

    static bool error_log_handler(command_vals_t v);
};

}

#endif
