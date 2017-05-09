#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "module.h"

namespace servx {

class Location {
public:
    Location(const std::string& s);

    Location(std::string&& s);

    const std::string& get_uri() const { return uri; }

    bool is_regex() const { return regex; }

    ModuleConf* get_conf(int index) { return confs[index]; }

private:
    bool regex;
    std::string uri;
    ModuleConf* confs[NULL_MODULE];
};

class LocationTree;

class LocationTreeNode {
    friend class LocationTree;

public:
    LocationTreeNode(): loc(nullptr) {}

    LocationTreeNode(const LocationTree&) = delete;

    LocationTree& operator=(const LocationTree&) = delete;

    ~LocationTreeNode() = default;

private:
    std::shared_ptr<Location> loc;
    std::unordered_map<char, std::shared_ptr<LocationTreeNode>> child;
};

class LocationTree {
public:
    LocationTree(): root(std::make_shared<LocationTreeNode>()) {}

    LocationTree(const LocationTree&) = delete;

    LocationTree& operator=(const LocationTree&) = delete;

    ~LocationTree() = default;

    bool push(std::string&& uri);

    std::shared_ptr<Location> search(const std::string& uri);

private:
    std::shared_ptr<LocationTreeNode> root;
};

}

#endif
