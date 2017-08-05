#include "location.h"

#include "http_module.h"
#include "module_manager.h"

namespace servx {

Location::Location(const std::string& s)
    : max_body_size(4096), regex(false), send_file(false),
      uri(s), handler(nullptr) {
    auto manager = ModuleManager::instance();
    manager->for_each_http([this](HttpModule* module) {
            confs[reinterpret_cast<uintptr_t>(module)] =
                std::unique_ptr<HttpConf>(module->create_loc_conf());
            return true;
        });
}

}
