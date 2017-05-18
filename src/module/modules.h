#ifndef _MODULES_H_
#define _MODULES_H_

namespace servx {

enum ModuleIndex {
    MAIN_CORE_MODULE,
    MAIN_EVENT_MODULE,
    EPOLL_MODULE,
    MAIN_HTTP_MODULE,
    HTTP_STATIC_MODULE,
    NULL_MODULE
};

}

#endif
