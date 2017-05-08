#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <string>
#include <unordered_map>

#include "module.h"

namespace servx {

class Location {
public:
    Location(std::string s);

    const std::string& get_uri() const { return uri; }

    bool is_regex() const { return regex; }

    ModuleConf* get_conf(int index) { return confs[index]; }

private:
    bool regex;
    std::string uri;
    ModuleConf* confs[NULL_MODULE];
};

class LocationTreeNode {
public:
    LocationTreeNode(): loc(nullptr) {}

    Location *loc;
    std::unordered_map<char, LocationTreeNode*> child;
};

class LocationTree {
public:
    LocationTree() = default;

    bool push(Location *loc);

    Location* search(const std::string uri);

private:
    LocationTreeNode root;
};

}

#endif
