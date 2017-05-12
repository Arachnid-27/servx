#include "core_module.h"

#include <sys/resource.h>

#include "cycle.h"
#include "logger.h"
#include "module_manager.h"

namespace servx {

bool MainCoreModule::init_conf() {
    conf.worker = 1;
    conf.daemon = true;
    conf.rlimit_nofile = -1;
    return true;
}

bool MainCoreModule::init_module() {
    if (conf.rlimit_nofile > 0) {
        rlimit rlmt;
        rlmt.rlim_cur = conf.rlimit_nofile;
        rlmt.rlim_max = conf.rlimit_nofile;

        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
            // error_log
        }
    }

    return true;
}

int MainCoreModule::worker_handler(command_vals_t v) {
    conf.worker = atoi(v[0].c_str());

    if (conf.worker <= 0) {
        return ERROR_COMMAND;
    }

    return NULL_BLOCK;
}

int MainCoreModule::rlimit_nofile_handler(command_vals_t v) {
    conf.rlimit_nofile = atoi(v[0].c_str());

    return NULL_BLOCK;
}

int MainCoreModule::daemon_handler(command_vals_t v) {
    conf.daemon = v[0] == "on" ? true : false;

    return NULL_BLOCK;
}

int MainCoreModule::error_log_handler(command_vals_t v) {
    for (auto &s : v) {
        Logger::instance()->push_file(s);
    }

    return NULL_BLOCK;
}

}
