#include "server.h"

#include "module_manager.h"

namespace servx {

HttpContext::HttpContext(bool srv) {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_HTTP_MODULE; ++i) {
        if (srv) {
            confs[i] = manager->get_module(i)->create_srv_conf();
        } else {
            confs[i] = manager->get_module(i)->create_loc_conf();
        }
    }
}

}
