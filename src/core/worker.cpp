#include "worker.h"

void worker_process_cycle() {
    auto manager = ModuleManager::instance();

    Clock::instance()->update();

    if (!manager->for_each([](Module* m) { return m->init_process(); })) {
        // error_log
        exit(EXIT_FAILURE);
    }

    while (1) {
    }
}
