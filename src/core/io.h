#ifndef _IO_H_
#define _IO_H_

#include "connection.h"

namespace servx {

int recv(Connection* conn, size_t sz);

}

#endif
