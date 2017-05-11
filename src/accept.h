#ifndef _ACCEPT_H_
#define _ACCEPT_H_

#include "event.h"

namespace servx {

void accept_event_handler(Event* ev);

bool enable_accept_event();

bool disable_accept_event();

}

#endif
