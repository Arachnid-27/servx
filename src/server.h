#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>

#include "location.h"

namespace servx {

class Server {
public:
    Server();

    void push_server_name(std::string s) { server_names.push_back(std::move(s)); }

    bool push_location(Location* loc);

    Location* serach(const std::string& uri);

    ModuleConf* get_conf(int index) { return confs[index]; }

private:
    std::vector<std::string> server_names;
    std::vector<Location*> regex_locations;
    LocationTree prefix_locations;
    ModuleConf* confs[NULL_MODULE];
};

}

#endif
