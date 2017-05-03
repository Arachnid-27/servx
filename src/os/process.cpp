#include "process.h"

bool Process::run() {
    switch (pid = fork()) {
    case -1:
        return false;
    case 0:
        printf("-----------\n");
        proc();
        printf("++++++++++n");
        exit(EXIT_FAILURE);
    default:
        break;
    }
    
    return true;
}
