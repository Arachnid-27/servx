#ifndef _CORE_H_
#define _CORE_H_

namespace servx {

enum ResultCode {
    SERVX_OK = 0,
    SERVX_AGAIN,
    SERVX_ERROR,
    SERVX_PARTIAL,
    SERVX_DONE,
    SERVX_DENY
};

}

#endif
