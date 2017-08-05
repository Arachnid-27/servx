#include "http_location_tree.h"

namespace servx {

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
