#include "event_module.h"

#include <sys/resource.h>

#include "connection_pool.h"
#include "module_manager.h"
#include "signals.h"

namespace servx {

bool MainEventModule::init_conf() {
    conf.time_resolution = 100;
    conf.connections = 1024;
    conf.multi_accept = false;
    return true;
}

bool MainEventModule::init_module() {
    rlimit rlmt;

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        // error_log
    } else {
        if (static_cast<unsigned long>(conf.connections) > rlmt.rlim_cur) {
            conf.connections = rlmt.rlim_cur;
            // error_log
        }
    }

    return true;
}

bool MainEventModule::init_process() {
    if (!signal(SIGALRM, sig_timer_handler)) {
        // err_log
        return false;
    }

    if (!set_timer(conf.time_resolution)) {
        // err_log
        return false;
    }

    ConnectionPool::instance()->init(conf.connections);

    return true;
}

int MainEventModule::event_handler(command_vals_t v) {
    return EVENT_BLOCK;
}

int MainEventModule::timer_resolution_handler(command_vals_t v) {
    conf.time_resolution = atoi(v[0].c_str());

    if (conf.connections <= 0) {
        return ERROR_COMMAND;
    }

    return NULL_BLOCK;
}

int MainEventModule::connections_handler(command_vals_t v) {
    conf.connections = atoi(v[0].c_str());

    if (conf.connections <= 0) {
        return ERROR_COMMAND;
    }

    return NULL_BLOCK;
}

int MainEventModule::multi_accept_handler(command_vals_t v) {
    conf.multi_accept = v[0] == "on";
    return NULL_BLOCK;
}

void MainEventModule::sig_timer_handler(int sig) {
    sig_timer_alarm = 1;
    // err_log
}

}
