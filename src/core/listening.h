#ifndef _LISTENING_H_
#define _LISTENING_H_


#include "event.h"

#include <memory>


class Listening {
private:
    int fd;
    std::shared_ptr<Event> read_event;
    std::shared_ptr<Event> write_event;
};


#endif
