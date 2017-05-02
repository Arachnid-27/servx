#ifndef _LISTENING_H_
#define _LISTENING_H_

#include "core.h"
#include "event.h"

using socket_t = int;

class Listening {
private:
    socket_t fd;
    std::shared_ptr<Event> read_event;
    std::shared_ptr<Event> write_event;
};

#endif
