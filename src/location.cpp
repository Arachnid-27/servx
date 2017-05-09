#include "location.h"

#include "module_manager.h"

namespace servx {

Location::Location(const std::string& s): uri(s) {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        confs[i] = manager->get_module(i)->create_loc_conf();
    }
}

Location::Location(std::string&& s): uri(std::move(s)) {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        confs[i] = manager->get_module(i)->create_loc_conf();
    }
}

bool LocationTree::push(std::string&& uri) {
    if (uri[0] != '/') {
        return false;
    }

    std::shared_ptr<LocationTreeNode> node = root;
    auto length = uri.length();

    for (size_t i = 1; i != length; ++i) {
        node = node->child[uri[i]];
    }

    if (node->loc != nullptr) {
        return false;
    }

    node->loc = std::make_shared<Location>(std::move(uri));
    return true;
}

std::shared_ptr<Location> LocationTree::search(const std::string& uri) {
    if (uri[0] != '/') {
        return nullptr;
    }

    std::shared_ptr<Location> result = nullptr;
    std::shared_ptr<LocationTreeNode> node = root;

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
