#include "location.h"

#include "module_manager.h"

namespace servx {

Location::Location(const std::string& s): uri(s) {
    auto manager = HttpModuleManager::instance();
    for (int i = 0; i < NULL_MODULE; ++i) {
        confs[i] = std::unique_ptr<ModuleConf>(
            manager->get_module(i)->create_loc_conf());
    }
}

bool LocationTree::push(const std::string& uri) {
    if (uri[0] != '/') {
        return false;
    }

    LocationTreeNode* node = root.get();
    auto length = uri.length();

    for (size_t i = 1; i != length; ++i) {
        node = (node->child[uri[i]]).get();
    }

    if (node->loc != nullptr) {
        return false;
    }

    node->loc = std::make_shared<Location>(uri);
    return true;
}

std::shared_ptr<Location> LocationTree::search(const std::string& uri) {
    if (uri[0] != '/') {
        return nullptr;
    }

    std::shared_ptr<Location> result = nullptr;
    LocationTreeNode* node = root.get();
    decltype(node->child.find(0)) it;

    for (auto ch : uri) {
        it = node->child.find(ch);
        if (it == node->child.end()) {
            break;
        }
        if (it->second->loc != nullptr) {
            result = it->second->loc;
        }
        node = (it->second).get();
    }

    return result;
}

}
