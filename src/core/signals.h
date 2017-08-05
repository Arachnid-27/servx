#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <signal.h>

namespace servx {

bool signal(int signum, void (*handler)(int));

bool set_timer(int value);

}

#endif
