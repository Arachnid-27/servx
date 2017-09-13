#include "worker.h"

#include <cstdlib>

#include "clock.h"
#include "event_module.h"
#include "logger.h"
#include "listener.h"
#include "module_manager.h"
#include "signals.h"
#include "timer.h"

namespace servx {

void worker_process_cycle() {
    auto manager = ModuleManager::instance();

    Logger::instance()->update_pid();
    Clock::instance()->update();

    if (!manager->for_each([](Module* m) { return m->init_process(); })) {
        Logger::instance()->error("init process error");
        exit(EXIT_FAILURE);
    }

    if (!Listener::instance()->open_listenings()) {
        Logger::instance()->error("init process error");
        exit(EXIT_FAILURE);
    }

    Logger::instance()->debug("open listenings success");

    while (true) {
        process_event();

        Timer::instance()->expire_timer();
    }
}

}
