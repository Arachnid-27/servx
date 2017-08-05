#include "signal_handler.h"

#include "logger.h"

namespace servx {

bool sig_timer_alarm = false;

void sig_timer_handler(int sig) {
    sig_timer_alarm = 1;
    Logger::instance()->debug("recv SIGALRM");
}

}
