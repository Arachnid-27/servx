#ifndef _HTTP_LOCATION_TREE_H_
#define _HTTP_LOCATION_TREE_H_

#include "location.h"

namespace servx {

class LocationTree;

class LocationTreeNode {
    friend class LocationTree;

public:
    LocationTreeNode(): loc(nullptr) {}

    LocationTreeNode(const LocationTree&) = delete;
    LocationTreeNode(const LocationTree&&) = delete;
    LocationTree& operator=(const LocationTree&) = delete;
    LocationTree& operator=(const LocationTree&&) = delete;

    ~LocationTreeNode() = default;

private:
    std::unique_ptr<Location> loc;
    std::unordered_map<char, std::unique_ptr<LocationTreeNode>> child;
};

class LocationTree {
public:
    LocationTree()
        : root(std::unique_ptr<LocationTreeNode>(new LocationTreeNode())) {}

    LocationTree(const LocationTree&) = delete;
    LocationTree(LocationTree&&) = delete;
    LocationTree& operator=(const LocationTree&) = delete;
    LocationTree& operator=(LocationTree&&) = delete;

    ~LocationTree() = default;

    bool push(Location* loc);

    Location* find(const std::string& uri);

private:
    std::unique_ptr<LocationTreeNode> root;
};

}

#endif
