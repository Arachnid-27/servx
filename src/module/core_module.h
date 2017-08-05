#ifndef _CORE_MODULE_
#define _CORE_MODULE_

#include "module.h"

namespace servx {

struct MainCoreConf {
    int rlimit_nofile;
    int worker;
    bool daemon;
};

class MainCoreModule: public CoreModule {
public:
    bool init_conf() override;

    bool init_module() override;

    static MainCoreConf conf;
    static MainCoreModule instance;
    static std::vector<Command*> commands;
};

namespace command {

class Worker: public Command {
public:
    Worker(): Command("core", "worker", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(&MainCoreModule::conf,
            &MainCoreConf::worker, v);
    }
};

class Daemon: public Command {
public:
    Daemon(): Command("core", "daemon", 1) {}

    bool execute(const command_args_t& v) override {
        return boolean_parse(&MainCoreModule::conf,
            &MainCoreConf::daemon, v);
    }
};

class RlimitNofile: public Command {
public:
    RlimitNofile(): Command("core", "rlimit_nofile", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(&MainCoreModule::conf,
            &MainCoreConf::rlimit_nofile, v);
    }
};

class ErrorLog: public Command {
public:
    ErrorLog(): Command("core", "error_log", -1) {}

    bool execute(const command_args_t& v) override;
};

}

}

#endif
