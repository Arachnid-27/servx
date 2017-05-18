#ifndef _HTTP_MODULE_
#define _HTTP_MODULE_

#include "core_module.h"
#include "inet.h"
#include "modules.h"
#include "server.h"

namespace servx {

class HttpModule: public Module {
public:
    virtual bool post_configuration() { return true; }

    virtual ModuleConf* create_srv_conf() { return nullptr; }

    virtual ModuleConf* create_loc_conf() { return nullptr; }

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

protected:
    HttpModuleWithConf(const std::initializer_list<Command*>& v)
        : ModuleWithConf<HttpModule, Main, Index>(v) {}
};

struct MainHttpConf {
    std::vector<std::unique_ptr<Server>> servers;
};

class MainHttpModule
    : public HttpModuleWithConf<MainHttpConf,
                                void,
                                void,
                                MAIN_HTTP_MODULE> {
public:
    MainHttpModule(): HttpModuleWithConf(
        {
            new Command(CORE_BLOCK,
                        "http",
                        lambda_handler(http_handler), 0,
                        lambda_post_handler(http_post_handler)),
            new Command(HTTP_BLOCK,
                        "server",
                        lambda_handler(server_handler), 0,
                        lambda_post_handler(server_post_handler)),
            new Command(SERVER_BLOCK,
                        "location",
                        lambda_handler(location_handler), 1),
            new Command(SERVER_BLOCK,
                        "address",
                        lambda_handler(address_handler), -1,
                        lambda_post_handler(address_post_handler)),
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
            new Command(LOCATION_BLOCK,
                        "client_max_body_size",
                        lambda_handler(client_max_body_size_handler), 1),
            new Command(LOCATION_BLOCK,
                        "root",
                        lambda_handler(root_handler), 1)
        }), loc(nullptr) {}

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

    int client_max_body_size_handler(command_vals_t v);

    int root_handler(command_vals_t v);

    bool http_post_handler();

    bool server_post_handler();

    bool address_post_handler();

    bool location_post_handler();

private:
    using tcp_socket_setter = void (TcpSocket::*)(int);

    int set_address_value(command_vals_t v, tcp_socket_setter setter);

private:
    // temp variables
    std::shared_ptr<TcpSocket> tcp_socket;
    std::string addr;
    std::string port;
    Location *loc;
    bool default_server;
};

}

#endif
