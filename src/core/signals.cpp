#include "signals.h"

#include <sys/time.h>

#include <cstring>

namespace servx {

bool signal(int signum, void (*handler)(int)) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    return sigaction(signum, &sa, NULL) != -1;
}

bool set_timer(int value) {
    struct itimerval itv;

    itv.it_interval.tv_sec = value / 1000;
    itv.it_interval.tv_usec = (value % 1000) * 1000;
    itv.it_value.tv_sec = value / 1000;
    itv.it_value.tv_usec = (value % 1000) * 1000;

    return setitimer(ITIMER_REAL, &itv, NULL) != -1;
}

}
