#include "core_module.h"

#include <sys/resource.h>

#include "logger.h"
#include "module_manager.h"

namespace servx {

bool MainCoreModule::init_conf() {
    conf->worker = 1;
    conf->daemon = true;
    conf->rlimit_nofile = -1;
    return true;
}

bool MainCoreModule::init_module() {
    if (conf->rlimit_nofile > 0) {
        rlimit rlmt;
        rlmt.rlim_cur = conf->rlimit_nofile;
        rlmt.rlim_max = conf->rlimit_nofile;

        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
            Logger::instance()->warn("setrlimit() failed, ignore");
        }
    }

    return true;
}

int MainCoreModule::daemon_handler(command_vals_t v) {
    conf->daemon = v[0] == "on";

    return NULL_BLOCK;
}

int MainCoreModule::error_log_handler(command_vals_t v) {
    for (auto &s : v) {
        Logger::instance()->push_file(s);
    }

    return NULL_BLOCK;
}

}
