#ifndef _CORE_MODULE_
#define _CORE_MODULE_

#include "module.h"
#include "modules.h"

namespace servx {

class CoreModule: public Module {
protected:
    CoreModule(const std::initializer_list<Command*>& v): Module(CORE_MODULE, v) {}
};

struct MainCoreConf: public ModuleConf {
    int rlimit_nofile;
    int worker;
    bool daemon;
};

class MainCoreModule: public ModuleWithConf<CoreModule,
                                            MainCoreConf,
                                            MAIN_CORE_MODULE> {
public:
    MainCoreModule(): ModuleWithConf(
        {
            new Command(CORE_BLOCK,
                        "worker",
                        lambda_handler(worker_handler), 1),
            new Command(CORE_BLOCK,
                        "daemon",
                        lambda_handler(daemon_handler), 1),
            new Command(CORE_BLOCK,
                        "rlimit_nofile",
                        lambda_handler(rlimit_nofile_handler), 1),
            new Command(CORE_BLOCK,
                        "error_log",
                        lambda_handler(error_log_handler))
        }) {}

    bool init_conf() override;

    bool init_module() override;

    int worker_handler(command_vals_t v) {
        return set_conf_int<MainCoreConf,
           &MainCoreConf::worker>(conf, v[0]);
    }

    int daemon_handler(command_vals_t v);

    int rlimit_nofile_handler(command_vals_t v) {
        return set_conf_int<MainCoreConf,
            &MainCoreConf::rlimit_nofile>(conf, v[0]);
    }

    int error_log_handler(command_vals_t v);
};

}

#endif
