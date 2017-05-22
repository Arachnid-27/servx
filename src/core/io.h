#ifndef _IO_H_
#define _IO_H_

#include <cstdint>
#include <sys/uio.h>

namespace servx {

int io_read(int fd, char* buf, uint32_t count);

int io_write(int fd, char* buf, uint32_t count);

int io_write_chain(int fd, struct iovec* iov, uint32_t count);

int io_sendfile(int out_fd, int in_fd, long* offset, uint32_t count);

}

#endif
