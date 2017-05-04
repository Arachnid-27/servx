#ifndef _LISTENING_H_
#define _LISTENING_H_

#include "event.h"

namespace servx {

class Listening {
private:
    int fd;
    Event* read_event;
    Event* write_event;
};

}

#endif
