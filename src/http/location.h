#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "module.h"

namespace servx {

class HttpRequest;

using http_content_handler_t = std::function<int(HttpRequest*)>;

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

    void set_content_handler(const http_content_handler_t& h) { handler = h; }
    http_content_handler_t get_content_handler() { return handler; }

    const std::string& get_root() const { return root; }
    void set_root(const std::string& s) { root = s; }

    bool is_send_file() const { return send_file; }
    void set_send_file(bool s) { send_file = s; }

    int get_max_body_size() const { return max_body_size; }
    void set_max_body_size(int sz) { max_body_size = sz; }

private:
    int max_body_size;
    bool regex;
    bool send_file;
    std::string root;
    std::string uri;

    http_content_handler_t handler;

    std::unordered_map<uintptr_t, std::unique_ptr<HttpConf>> confs;
};

template <typename T>
inline typename T::loc_conf_t* Location::get_conf() {
    return static_cast<typename T::loc_conf_t*>(
        confs[reinterpret_cast<uintptr_t>(&T::instance)].get());
}

}

#endif
