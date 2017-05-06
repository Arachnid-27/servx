#include "process.h"

#include <unistd.h>

#include <cstdlib>

namespace servx {

bool Process::run() {
    switch (pid = fork()) {
    case -1:
        return false;
    case 0:
        proc();
        exit(EXIT_FAILURE);
    default:
        break;
    }
    
    return true;
}

}
