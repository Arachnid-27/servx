#include "event_module.h"

#include <sys/resource.h>

#include "connection_pool.h"
#include "logger.h"
#include "signals.h"
#include "signal_handler.h"

namespace servx {

EventCoreConf EventCoreModule::conf;
EventCoreModule EventCoreModule::instance;
std::vector<Command*> EventCoreModule::commands = {
    new command::Event,
    new command::TimerResolution,
    new command::Connections,
    new command::MultiAccept
};

bool EventCoreModule::init_conf() {
    conf.time_resolution = 100;
    conf.connections = 1024;
    conf.multi_accept = false;
    conf.module = nullptr;
    return true;
}

bool EventCoreModule::init_module() {
    rlimit rlmt;

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        Logger::instance()->warn("get rlimit error");
    } else {
        if (static_cast<uint64_t>(conf.connections) > rlmt.rlim_cur) {
            conf.connections = rlmt.rlim_cur;
            Logger::instance()->warn("set rlimit error");
        }
    }

    return true;
}

bool EventCoreModule::init_process() {
    if (!signal(SIGALRM, sig_timer_handler)) {
        Logger::instance()->error("register SIGALRM error");
        return false;
    }

    Logger::instance()->debug("register SIGALRM success");

    if (!set_timer(conf.time_resolution)) {
        Logger::instance()->error("set timer error");
        return false;
    }

    Logger::instance()->debug("set timer success, resolution is %d",
                              conf.time_resolution);

    ConnectionPool::instance()->init(conf.connections);

    Logger::instance()->debug("init connections pool success, %d in total",
                              conf.connections);

    return true;
}

}
