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

    Location(const Location&) = delete;
    Location(const Location&&) = delete;
    Location& operator=(const Location&) = delete;
    Location& operator=(const Location&&) = delete;

    const std::string& get_uri() const { return uri; }

    bool is_regex() const { return regex; }

    ModuleConf* get_conf(int index) { return confs[index].get(); }

private:
    bool regex;
    std::string uri;
    std::unique_ptr<ModuleConf> confs[NULL_MODULE];
};

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
    std::shared_ptr<Location> loc;
    std::unordered_map<char, std::unique_ptr<LocationTreeNode>> child;
};

class LocationTree {
public:
    LocationTree(): root(std::unique_ptr<LocationTreeNode>()) {}

    LocationTree(const LocationTree&) = delete;

    LocationTree& operator=(const LocationTree&) = delete;

    ~LocationTree() = default;

    bool push(const std::string& uri);

    std::shared_ptr<Location> search(const std::string& uri);

private:
    std::unique_ptr<LocationTreeNode> root;
};

}

#endif
