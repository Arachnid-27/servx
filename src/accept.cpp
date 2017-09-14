#include "accept.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection_pool.h"
#include "event_module.h"
#include "listener.h"
#include "logger.h"
#include "module_manager.h"
#include "timer.h"

namespace servx {

void accept_event_handler(Listening* lst, Event* ev) {
    if (ev->is_timeout()) {
        // if we get ENFILE or EMFILE
        // it means we can't accept until we release some fds
        // so we will disable accept events and set a timer
        // when the timer expire we enable accept events and try again
        if (!Listener::instance()->enable_all()) {
            Logger::instance()->info("enable all listening fd");
            return;
        }
    }

    bool multi_accept = EventCoreModule::conf.multi_accept;
    Connection *conn = ev->get_connection();

//    sockaddr sa;
//    socklen_t len;
    int fd, err;

    ev->set_ready(false);

    Logger::instance()->debug("call accept4...");

    while (true) {
        fd = accept4(conn->get_fd(), NULL, NULL, SOCK_NONBLOCK);

        Logger::instance()->debug("accept4 return, get %d", fd);

        if (fd == -1) {
            err = errno;

            switch (err) {
            case EAGAIN:
                Logger::instance()->info("accept4() not ready");
                break;
            case ECONNABORTED:
                if (multi_accept) {
                    continue;
                }
                break;
            case EMFILE:
            case ENFILE:
                if (Listener::instance()->disable_all()) {
                    Logger::instance()->info("disable all listening fd");
                    Timer::instance()->add_timer(ev, 1000);
                }
                break;
            }

            return;
        }

        Connection *new_conn = ConnectionPool::instance()->get_connection(fd);
        if (new_conn == nullptr) {
            Logger::instance()->warn("can not get connection");
            if (::close(fd) == -1) {
                Logger::instance()->warn("close fd %d failed", fd);
            }
            return;
        }

        new_conn->get_write_event()->set_ready(true); // enable write event
        if (lst->get_socket()->is_deferred_accept()) {
            new_conn->get_read_event()->set_ready(true);
        }

        lst->handle(new_conn);

        if (!multi_accept) {
            break;
        }
    }
}

}
