#ifndef _MODULES_H_
#define _MODULES_H_

namespace servx {

enum ModuleIndex {
    MAIN_CORE_MODULE,
    MAIN_EVENT_MODULE,
    EPOLL_MODULE,
    HTTP_CORE_MODULE,
    HTTP_STATIC_MODULE,
    HTTP_UPSTREAM_MODULE,
    HTTP_PROXY_MODULE,
    NULL_MODULE
};

}

#endif
