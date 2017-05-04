#include "event_module.h"

#include <sys/resource.h>

#include "module_manager.h"

namespace servx {

bool MainEventModule::init_conf() {
    conf.time_resolution = 100;
    conf.connections = 1024;
    return true;
}

bool MainEventModule::init_module() {
    rlimit rlmt;

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        // error_log
    } else {
        if (conf.connections > rlmt.rlim_cur) {
            conf.connections = rlmt.rlim_cur;
            // error_log
        }
    }
    
    return true;
}

bool MainEventModule::init_process() {
    return true;
}

bool MainEventModule::event_handler(command_vals_t v) {
    return true;
}

bool MainEventModule::timer_resolution_handler(command_vals_t v) {
    auto conf = ModuleManager::instance()->get_conf<MainEventModule>();
    conf->time_resolution = atoi(v[0].c_str());

    if (conf->connections <= 0) {
        return false;
    }

    return true;
}

bool MainEventModule::connections_handler(command_vals_t v) {
    auto conf = ModuleManager::instance()->get_conf<MainEventModule>();
    conf->connections = atoi(v[0].c_str());

    if (conf->connections <= 0) {
        return false;
    }

    return true;
}

}
