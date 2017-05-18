#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "module.h"
#include "modules.h"

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

    uint32_t get_client_max_body_size() const { return client_max_body_size; }
    void set_client_max_body_size(uint32_t n) { client_max_body_size = n; }

    const std::string& get_root() const { return root; }
    void set_root(const std::string& s) { root = s; }
    void set_root(std::string&& s) { root = std::move(s); }

private:
    bool regex;
    std::string root;
    std::string uri;
    std::unique_ptr<ModuleConf> confs[NULL_MODULE];
    uint32_t client_max_body_size;
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
