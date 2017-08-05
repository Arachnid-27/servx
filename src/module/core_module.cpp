#include "core_module.h"

#include <sys/resource.h>

#include "logger.h"

namespace servx {

MainCoreConf MainCoreModule::conf;
MainCoreModule MainCoreModule::instance;
std::vector<Command*> MainCoreModule::commands = {
    new command::Worker,
    new command::Daemon,
    new command::RlimitNofile,
    new command::ErrorLog
};

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
            Logger::instance()->warn("setrlimit() failed, ignore");
        }
    }
    return true;
}

namespace command {

bool ErrorLog::execute(const command_args_t& v) {
    for (auto &s : v) {
        Logger::instance()->push_file(s);
    }
    return true;
}

}

}
