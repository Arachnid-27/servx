#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>
#include <string>

#include "inet.h"
#include "http_module.h"

namespace servx {

class HttpContext {
public:
    HttpContext(bool srv);

    HttpModuleConf* get_conf(int index) { return confs[index]; }

private:
    HttpModuleConf* confs[NULL_HTTP_MODULE];
};

class Location: public HttpContext {
public:
    Location(const char* s): uri(s) {}

    Location(std::string s): uri(s) {}

    Location(Location&&) = default;

private:
    std::string uri;
};

class Server: public HttpContext {
public:
    void push_server_name(std::string s) { server_names.push_back(std::move(s)); }

    void push_location(Location&& loc) { locations.push_back(std::move(loc)); }

    void new_address() { addresses.emplace_back(); }

    IPAddress& get_last_address() { return addresses.back(); }

private:
    std::vector<std::string> server_names;
    std::vector<IPAddress> addresses;
    std::vector<Location> locations;
};

}

#endif
