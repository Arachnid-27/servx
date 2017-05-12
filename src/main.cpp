#include <cstring>

#include "core_module.h"
#include "cycle.h"
#include "daemon.h"
#include "logger.h"
#include "master.h"
#include "module_manager.h"
#include "worker.h"

using namespace servx;

int main(int argc, char* argv[]) {
    auto cycle = Cycle::instance();

    cycle->reload();

    Logger::instance()->debug("parse configure success!");

    Logger::instance()->open_files();

    auto conf = ModuleManager::instance()->get_conf<MainCoreModule>();

    if (conf->daemon) {
        daemonize();
    }

    Logger::instance()->debug("prepare to spawn process...");

    memcpy(argv[0], "servx-worker", sizeof("servx-worker"));

    for (auto i = 0; i < conf->worker; ++i) {
        auto process = cycle->spawn_process(worker_process_cycle);
        if (!process.run()) {
            exit(EXIT_FAILURE);
        }
    }

    memcpy(argv[0], "servx-master", sizeof("servx-master"));

    master_process_cycle();

    return 0;
}
