#ifndef _HTTP_MODULE_
#define _HTTP_MODULE_

#include "module.h"
#include "server.h"

namespace servx {

struct HttpMainConf {
    std::vector<std::unique_ptr<Server>> servers;
    std::unique_ptr<Server> temp_server;
    std::unique_ptr<Location> temp_location;
    std::unique_ptr<TcpListenSocket> temp_socket;
    bool temp_default;
};

class HttpMainModule: public HttpModule {
public:
    using srv_conf_t = void;
    using loc_conf_t = void;

    static HttpMainConf conf;
    static HttpMainModule instance;
    static std::vector<Command*> commands;
};

namespace command {

class Http: public Command {
public:
    Http(): Command("core", "http", 0) {}

    bool execute(const command_args_t& v) override { return true; }

    bool post_execute() override;
};

class HServer: public Command {
public:
    HServer(): Command("http", "server", 0) {}

    bool execute(const command_args_t& v) override;

    bool post_execute() override;
};

class HLocation: public Command {
public:
    HLocation(): Command("server", "location", 1) {}

    bool execute(const command_args_t& v) override;

    bool post_execute() override;
};

class Address: public Command {
public:
    Address(): Command("server", "address", -1) {}

    bool execute(const command_args_t& v) override;

    bool post_execute() override;
};

class ServerName: public Command {
public:
    ServerName(): Command("server", "server_name", -1) {}

    bool execute(const command_args_t& v) override;
};

class Timeout: public Command {
public:
    Timeout(): Command("server", "timeout", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(HttpMainModule::conf.temp_server.get(),
            &Server::set_timeout, v);
    }
};

class BufferSize: public Command {
public:
    BufferSize(): Command("server", "buffer_size", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(HttpMainModule::conf.temp_server.get(),
            &Server::set_buffer_size, v);
    }
};

class Addr: public Command {
public:
    Addr(): Command("address", "addr", 1) {}

    bool execute(const command_args_t& v) override {
        HttpMainModule::conf.temp_socket->set_addr_str(v[0]);
        return true;
    }
};

class Port: public Command {
public:
    Port(): Command("address", "port", 1) {}

    bool execute(const command_args_t& v) override {
        HttpMainModule::conf.temp_socket->set_port_str(v[0]);
        return true;
    }
};

class Backlog: public Command {
public:
    Backlog(): Command("address", "backlog", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(HttpMainModule::conf.temp_socket.get(),
            &TcpListenSocket::set_backlog, v);
    }
};

class SendBuf: public Command {
public:
    SendBuf(): Command("address", "send_buf", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(static_cast<TcpSocket*>(
            HttpMainModule::conf.temp_socket.get()),
            &TcpSocket::set_send_buf, v);
    }
};

class RecvBuf: public Command {
public:
    RecvBuf(): Command("address", "recv_buf", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(static_cast<TcpSocket*>(
            HttpMainModule::conf.temp_socket.get()),
            &TcpSocket::set_recv_buf, v);
    }
};

class MaxBodySize: public Command {
public:
    MaxBodySize(): Command("location", "max_body_size", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(HttpMainModule::conf.temp_location.get(),
            &Location::set_max_body_size, v);
    }
};

class Root: public Command {
public:
    Root(): Command("location", "root", 1) {}

    bool execute(const command_args_t& v) override;
};

class Sendfile: public Command {
public:
    Sendfile(): Command("location", "sendfile", 1) {}

    bool execute(const command_args_t& v) override {
        return boolean_parse(HttpMainModule::conf.temp_location.get(),
            &Location::set_send_file, v);
    }
};

}

}

#endif
