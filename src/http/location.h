#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "module.h"
#include "modules.h"

namespace servx {

struct HttpCoreLocConf: public ModuleConf {
    int client_max_body_size;
};

class Location {
public:
    explicit Location(const std::string& s);

    Location(const Location&) = delete;
    Location(const Location&&) = delete;
    Location& operator=(const Location&) = delete;
    Location& operator=(const Location&&) = delete;

    const std::string& get_uri() const { return uri; }

    bool is_regex() const { return regex; }

    template <typename T>
    typename T::loc_conf_t* get_conf();

    HttpCoreLocConf* get_core_conf();

    const std::string& get_root() const { return root; }
    void set_root(const std::string& s) { root = s; }
    void set_root(std::string&& s) { root = std::move(s); }

    bool is_send_file() const { return send_file; }
    void set_send_file(bool s) { send_file = s; }

private:
    uint32_t regex:1;
    uint32_t send_file:1;

    std::string root;
    std::string uri;
    std::unique_ptr<ModuleConf> confs[NULL_MODULE];
};

template <typename T>
inline typename T::loc_conf_t* Location::get_conf() {
    return static_cast<typename T::loc_conf_t*>(confs[T::index].get());
}

inline HttpCoreLocConf* Location::get_core_conf() {
    return static_cast<HttpCoreLocConf*>(confs[HTTP_CORE_MODULE].get());
}

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
