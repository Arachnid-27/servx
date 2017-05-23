#include "event_module.h"

#include <sys/resource.h>

#include "connection_pool.h"
#include "logger.h"
#include "signals.h"

namespace servx {

bool MainEventModule::init_conf() {
    conf->time_resolution = 100;
    conf->connections = 1024;
    conf->multi_accept = false;
    conf->module = nullptr;
    return true;
}

bool MainEventModule::init_module() {
    rlimit rlmt;

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        Logger::instance()->warn("get rlimit error");
    } else {
        if (static_cast<unsigned long>(conf->connections) > rlmt.rlim_cur) {
            conf->connections = rlmt.rlim_cur;
            Logger::instance()->warn("set rlimit error");
        }
    }

    return true;
}

bool MainEventModule::init_process() {
    if (!signal(SIGALRM, sig_timer_handler)) {
        Logger::instance()->error("register SIGALRM error");
        return false;
    }

    Logger::instance()->debug("register SIGALRM success");

    if (!set_timer(conf->time_resolution)) {
        Logger::instance()->error("set timer error");
        return false;
    }

    Logger::instance()->debug("set timer success, resolution is %d",
                              conf->time_resolution);

    ConnectionPool::instance()->init(conf->connections);

    Logger::instance()->debug("init connections pool success, %d in total",
                              conf->connections);

    return true;
}

int MainEventModule::event_handler(command_vals_t v) {
    return EVENT_BLOCK;
}

int MainEventModule::multi_accept_handler(command_vals_t v) {
    conf->multi_accept = v[0] == "on";
    return NULL_BLOCK;
}

void MainEventModule::sig_timer_handler(int sig) {
    sig_timer_alarm = 1;
    Logger::instance()->debug("recv SIGALRM");
}

}
