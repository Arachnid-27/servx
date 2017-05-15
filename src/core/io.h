#ifndef _IO_H_
#define _IO_H_

#include "connection.h"

namespace servx {

enum IOResultCode {
    IO_SUCCESS,
    IO_FINISH,
    IO_ERROR,
    IO_BLOCK,
    IO_BUF_TOO_SMALL
};

int recv(Connection* conn, Buffer* buf);

}

#endif
