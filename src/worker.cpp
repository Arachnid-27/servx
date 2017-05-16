#include "worker.h"

#include <cstdlib>

#include "clock.h"
#include "event_module.h"
#include "logger.h"
#include "listener.h"
#include "module_manager.h"

namespace servx {

void worker_process_cycle() {
    auto manager = ModuleManager::instance();

    Clock::instance()->update();

    if (!manager->for_each([](Module* m) { return m->init_process(); })) {
        Logger::instance()->error("init process error");
        exit(EXIT_FAILURE);
    }

    Listener::instance()->open_listenings();

    Logger::instance()->debug("open listenings success");

    while (true) {
        process_event();
    }
}

}
