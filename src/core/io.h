#ifndef _IO_H_
#define _IO_H_

#include <list>

#include "buffer.h"

namespace servx {

int io_recv(int fd, Buffer* buf);

int io_send(int fd, Buffer* buf);

int io_send_chain(int fd, std::list<Buffer>& chain);

}

#endif
