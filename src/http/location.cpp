#include "location.h"

#include "http_module.h"
#include "module_manager.h"

namespace servx {

Location::Location(const std::string& s)
    : regex(false), send_file(false), handler(nullptr), uri(s) {
    auto manager = ModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        auto module = manager->get_module(i);
        if (module != nullptr && module->get_type()== HTTP_MODULE) {
            confs[i] = std::unique_ptr<ModuleConf>(
                static_cast<HttpModule*>(module)->create_loc_conf());
        }
    }
}

bool LocationTree::push(Location* loc) {
    auto &uri = loc->get_uri();

    if (uri[0] != '/') {
        return false;
    }

    LocationTreeNode* node = root.get();
    auto length = uri.length();
    decltype(node->child.find(' ')) it;

    for (size_t i = 0; i < length; ++i) {
        it = node->child.find(uri[i]);
        if (it == node->child.end()) {
            node->child[uri[i]] =
                std::unique_ptr<LocationTreeNode>(new LocationTreeNode());
        }
        node = (node->child[uri[i]]).get();
    }

    if ((node->loc) != nullptr) {
        return false;
    }

    node->loc = std::unique_ptr<Location>(loc);
    return true;
}

Location* LocationTree::find(const std::string& uri) {
    if (uri[0] != '/') {
        return nullptr;
    }

    Location* result = nullptr;
    LocationTreeNode* node = root.get();
    auto length = uri.length();
    decltype(node->child.find(' ')) it;

    for (size_t i = 0; i < length; ++i) {
        it = node->child.find(uri[i]);
        if (it == node->child.end()) {
            break;
        }
        if (it->second->loc != nullptr) {
            result = it->second->loc.get();
        }
        node = (it->second).get();
    }

    return result;
}

}
