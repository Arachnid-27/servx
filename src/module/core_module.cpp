#include "core_module.h"


bool MainCoreModule::init_conf() {
    conf->worker = 1;
    conf->daemon = 1;
    return true;
}


bool MainCoreModule::worker_handler(command_vals_t v) {
    auto conf = ModuleManager::instance()->get_conf<MainCoreModule>();
    conf->worker = atoi(v[0].c_str());

    if (conf->worker <= 0) {
        return false;
    }

    return true;
}


bool MainCoreModule::daemon_handler(command_vals_t v) {
    auto conf = ModuleManager::instance()->get_conf<MainCoreModule>();
    conf->daemon = v[0] == "on" ? true : false;

    return true;
}


bool MainCoreModule::error_log_handler(command_vals_t v) {
    auto cycle = Cycle::instance();

    for (auto s : v) {
        cycle->open_file(s);
    }

    return true;
}
