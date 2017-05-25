#include "process_manager.h"

#include <unistd.h>

#include "logger.h"

namespace servx {

ProcessManager* ProcessManager::manager = new ProcessManager;

bool ProcessManager::spawn() {
    cpu_set_t *set = nullptr;
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);

    if (cpus == -1) {
        Logger::instance()->warn("get cpu number failed");
    } else if (cpus == 1) {
        Logger::instance()->info("the number of cpu is 1");
    } else {
        set = CPU_ALLOC(cpus);
    }

    for (uint32_t i = 0; i < processes.size(); ++i) {
        if (cpus > 1) {
            CPU_ZERO_S(cpus, set);
            CPU_SET(i % cpus, set);
        }
        if (!processes[i].run(set)) {
            return false;
        }
    }

    if (set != nullptr) {
        CPU_FREE(set);
    }

    return true;
}

}
