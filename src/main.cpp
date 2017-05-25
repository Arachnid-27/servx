#include <cstring>

#include "clock.h"
#include "core_module.h"
#include "conf_parser.h"
#include "daemon.h"
#include "listener.h"
#include "logger.h"
#include "master.h"
#include "module_manager.h"
#include "process_manager.h"
#include "signals.h"
#include "worker.h"

using namespace servx;

static bool init();

int main(int argc, char* argv[]) {
    if (!init()) {
        Logger::instance()->alert("exit servx...");
        exit(EXIT_FAILURE);
    }

    auto conf = ModuleManager::instance()->get_conf<MainCoreModule>();

    if (conf->daemon) {
        daemonize();
    }

    servx::signal(SIGPIPE, SIG_IGN);

    for (auto i = 0; i < conf->worker; ++i) {
        ProcessManager::instance()->push(worker_process_cycle);
    }

    memcpy(argv[0], "servx-worker", sizeof("servx-worker"));

    if (!ProcessManager::instance()->spawn()) {
        exit(EXIT_FAILURE);
    }

    memcpy(argv[0], "servx-master", sizeof("servx-master"));

    master_process_cycle();

    return 0;
}

bool init() {
    Clock::instance()->update();

    auto manager = ModuleManager::instance();

    if (!manager->for_each([](Module* m) { return m->init_conf(); })) {
        Logger::instance()->error("init conf error!");
        return false;
    }

    auto parser = ConfParser::instance();

    parser->open("servx.conf");

    if (!parser->parse()) {
        Logger::instance()->error("parse configure error!");
        return false;
    }

    Logger::instance()->debug("parse configure success!");
    Logger::instance()->open_files();

    Listener::instance()->init_listenings();

    if (!manager->for_each([](Module* m) { return m->init_module(); })) {
        Logger::instance()->error("init module error!");
        return false;
    }

    return true;
}
