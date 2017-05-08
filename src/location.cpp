#include "location.h"

#include "module_manager.h"

namespace servx {

Location::Location(std::string s): uri(s) {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        confs[i] = manager->get_module(i)->create_loc_conf();
    }
}

bool LocationTree::push(Location *loc) {
    auto &uri = loc->get_uri();

    if (uri[0] != '/') {
        return false;
    }

    LocationTreeNode *node = &root;
    auto length = uri.length();

    for (size_t i = 1; i != length; ++i) {
        node = node->child[uri[i]];
    }

    if (node->loc != nullptr) {
        return false;
    }

    node->loc = loc;
    return true;
}

Location* LocationTree::search(const std::string uri) {
    if (uri[0] != '/') {
        return nullptr;
    }

    Location *result = nullptr;
    LocationTreeNode *node = &root;

    for (auto ch : uri) {
        auto it = node->child.find(ch);
        if (it == node->child.end()) {
            break;
        }
        if (it->second->loc != nullptr) {
            result = it->second->loc;
        }
        node = it->second;
    }

    return result;
}

}
