#ifndef _IO_H_
#define _IO_H_

#include "connection.h"

namespace servx {

enum IOResultCode {
    IO_SUCCESS,     // read/write success
    IO_PARTIAL,     // success but partial read/write
    IO_FINISH,      // read EOF
    IO_ERROR,
    IO_BLOCK,
    IO_BUF_TOO_SMALL
};

int recv(Connection* conn);

int send(Connection* conn, char* buf, uint32_t size);

}

#endif
