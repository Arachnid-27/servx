#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <signal.h>

namespace servx {

extern bool sig_timer_alarm;

bool signal(int signum, void (*handler)(int));

bool set_timer(int value);

}

#endif
