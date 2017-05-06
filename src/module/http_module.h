#ifndef _HTTP_MODULE_
#define _HTTP_MODULE_

#include "core_module.h"
#include "server.h"

namespace servx {

class HttpModuleConf {};

class HttpModule: public Module {
public:
    virtual HttpModuleConf* create_main_conf() { return nullptr; }

    virtual HttpModuleConf* create_srv_conf() { return nullptr; }

    virtual HttpModuleConf* create_loc_conf() { return nullptr; }

protected:
    HttpModule(const std::initializer_list<Command*>& v)
        : Module(HTTP_MODULE, v) {}
};

template <typename Main, typename Srv, typename Loc, int Index>
class HttpModuleWithConf: public ModuleWithConf<HttpModule, Main, Index> {
public:
    using main_conf_t = Main;
    using srv_conf_t = Srv;
    using loc_conf_t = Loc;

    enum { index = Index };

protected:
    HttpModuleWithConf(const std::initializer_list<Command*>& v)
        : HttpModule(v) {}
};

struct MainHttpConf {
    std::vector<Server> servers;
};

class MainHttpModule: public ModuleWithConf<HttpModule,
                                            MainHttpConf,
                                            MAIN_HTTP_MODULE> {
public:
    MainHttpModule(): ModuleWithConf(
        {
            new Command(CORE_BLOCK,
                        "http",
                        lambda_handler(http_handler), 0),
            new Command(HTTP_BLOCK,
                        "server",
                        lambda_handler(server_handler), 0,
                        lambda_post_handler(server_post_handler)),
            new Command(SERVER_BLOCK,
                        "location",
                        lambda_handler(location_handler), 1),
            new Command(SERVER_BLOCK,
                        "address",
                        lambda_handler(address_handler), 0),
            new Command(SERVER_BLOCK,
                        "server_name",
                        lambda_handler(server_name_handler), -1),
            new Command(ADDRESS_BLOCK,
                        "addr",
                        lambda_handler(addr_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "port",
                        lambda_handler(port_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "backlog",
                        lambda_handler(backlog_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "send_buf",
                        lambda_handler(send_buf_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "recv_buf",
                        lambda_handler(recv_buf_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "reuseport",
                        lambda_handler(reuseport_handler), 0),
        }) {}

    int http_handler(command_vals_t v);

    int server_handler(command_vals_t v);

    int location_handler(command_vals_t v);

    int address_handler(command_vals_t v);

    int server_name_handler(command_vals_t v);

    int addr_handler(command_vals_t v);

    int port_handler(command_vals_t v);

    int backlog_handler(command_vals_t v);

    int send_buf_handler(command_vals_t v);

    int recv_buf_handler(command_vals_t v);

    int reuseport_handler(command_vals_t v);

    bool server_post_handler();

private:
    using address_setter = void (IPAddress::*)(int);

    int set_address_value(command_vals_t v, address_setter setter);
};

}

#endif
