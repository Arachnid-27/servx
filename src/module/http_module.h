#ifndef _HTTP_MODULE_
#define _HTTP_MODULE_

#include "core_module.h"

namespace servx {

class HttpModule: public Module {
public:
    virtual bool post_configuration() { return true; }

    virtual ModuleConf* create_srv_conf() = 0;

    virtual ModuleConf* create_loc_conf() = 0;

protected:
    HttpModule(const std::initializer_list<Command*>& v)
        : Module(HTTP_MODULE, v) {}
};

template <typename Main, typename Srv, typename Loc, int Index>
class HttpModuleWithConf: public ModuleWithConf<HttpModule, Main, Index> {
public:
    using main_conf_t = Main;
    using srv_conf_t = Srv;
    using loc_conf_t = Loc;

    ModuleConf* create_srv_conf() override {
        return __conf_creator<Srv, ModuleConf>().create();
    }

    ModuleConf* create_loc_conf() override {
        return __conf_creator<Loc, ModuleConf>().create();
    }

protected:
    HttpModuleWithConf(const std::initializer_list<Command*>& v)
        : ModuleWithConf<HttpModule, Main, Index>(v) {}
};

}

#endif
