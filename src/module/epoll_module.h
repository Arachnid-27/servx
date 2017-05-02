#ifndef _EPOLL_MODULE_H_
#define _EPOLL_MODULE_H_

#include "core.h"

class EpollModuleConf {
};

class EpollModule:
    public EventModule<EpollModuleConf, ModuleIndex::EPOLL_MODULE> {
public:
    EpollModule():
        EventModule({
                    new Command("epoll", nullptr),
                   },
                   new EpollModuleConf) {}
};

#endif
