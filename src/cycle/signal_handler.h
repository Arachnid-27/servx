#ifndef _SIGNAL_HANDLER_H_
#define _SIGNAL_HANDLER_H_

namespace servx {

extern bool sig_timer_alarm;

void sig_timer_handler(int sig);

}

#endif

