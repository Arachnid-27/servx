#ifndef _EVENT_H_
#define _EVENT_H_

#include "core.h"

class Event {
private:
    unsigned write:1;
    unsigned event_set:1;
    unsigned timer_set:1;
    unsigned ready:1;
    unsigned timeout:1;
    std::function<void(std::shared_ptr<Event>)> handler;
};

#endif
