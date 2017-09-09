#ifndef _MODULE_H_
#define _MODULE_H_

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "command.h"
#include "core.h"
#include "event.h"

namespace servx {

class Module {
public:
    virtual bool init_conf() { return true; }

    virtual bool init_module() { return true; }

    virtual bool init_process() { return true; }

    virtual bool exit_process() { return true; }

    virtual bool exit_module() { return true; }

    virtual ~Module() = default;

protected:
    Module() = default;
};

class CoreModule: public Module {
};

class EventModule: public Module {
public:
    virtual bool add_event(Event* ev) = 0;

    virtual bool del_event(Event* ev) = 0;

    virtual bool add_connection(Connection* c) = 0;

    virtual bool del_connection(Connection* c) = 0;

    virtual bool process_events() = 0;
};

class HttpConf {
public:
    virtual ~HttpConf() = default;
};

class HttpModule: public Module {
public:
    virtual bool post_configuration() { return true; }

    virtual HttpConf* create_srv_conf() { return nullptr; }

    virtual HttpConf* create_loc_conf() { return nullptr; }
};

}

#endif
