#include "process.h"

#include <unistd.h>

#include <cstdlib>

namespace servx {

bool Process::run(cpu_set_t *set) {
    switch (pid = fork()) {
    case -1:
        return false;
    case 0:
        if (set != nullptr) {
            // we don't care if it success
            sched_setaffinity(getpid(), sizeof(cpu_set_t), set);
        }
        proc();
        exit(EXIT_FAILURE);
    default:
        return true;
    }
}

}
