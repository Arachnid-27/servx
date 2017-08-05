#include "epoll_module.h"

#include <errno.h>

#include "clock.h"
#include "logger.h"
#include "module_manager.h"
#include "signals.h"
#include "signal_handler.h"

namespace servx {

EpollModuleConf EpollModule::conf;
EpollModule EpollModule::instance;
std::vector<Command*> EpollModule::commands = {
    new command::UseEpoll,
    new command::EpollEvents
};

bool EpollModule::init_conf() {
    conf.epoll_events = 512;
    return true;
}

bool EpollModule::init_process() {
    if (conf.epoll_events <= 0) {
        return false;
    }

    ep = epoll_create(1);
    if (ep == -1) {
        return false;
    }

    event_list = new epoll_event[conf.epoll_events];

    return true;
}

bool EpollModule::add_event(Event* ev) {
    int op, prev;
    epoll_event ee;
    bool active = false;
    Connection *c = ev->get_connection();

    if (ev->is_write_event()) {
        active = c->get_read_event()->is_active();
        prev = EPOLLIN | EPOLLRDHUP;
        ee.events = EPOLLOUT;
    } else {
        active = c->get_write_event()->is_active();
        prev = EPOLLOUT;
        ee.events = EPOLLIN | EPOLLRDHUP;
    }

    if (active) {
        op = EPOLL_CTL_MOD;
        ee.events |= prev;
    } else {
        op = EPOLL_CTL_ADD;
    }

    // EPOLLEXCLUSIVE since Linux 4.5.0

    if (!c->is_listen()) {
        ee.events |= EPOLLET;
    }

    ee.data.ptr = c;

    if (epoll_ctl(ep, op, c->get_fd(), &ee) == -1) {
        return false;
    }

    ev->set_active(true);

    return true;
}

bool EpollModule::del_event(Event* ev) {
    Connection *c = ev->get_connection();

    if (c->is_close()) {
        ev->set_active(false);
        return true;
    }

    int op, prev;
    epoll_event ee;
    bool active = false;

    if (ev->is_write_event()) {
        active = c->get_read_event()->is_active();
        prev = EPOLLIN | EPOLLRDHUP;
    } else {
        active = c->get_write_event()->is_active();
        prev = EPOLLOUT;
    }

    if (active) {
        op = EPOLL_CTL_MOD;
        ee.events = prev;
        ee.data.ptr = c;

        if (!c->is_listen()) {
            ee.events |= EPOLLET;
        }
    } else {
        op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = NULL;
    }

    if (epoll_ctl(ep, op, c->get_fd(), &ee) == -1) {
        Logger::instance()->warn("epoll ctl error, %d", errno);
        return false;
    }

    ev->set_active(false);

    return true;
}

bool EpollModule::add_connection(Connection* c) {
    epoll_event ee;

    ee.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ee.data.ptr = c;

    if (!c->is_listen()) {
        ee.events |= EPOLLET;
    }

    if (epoll_ctl(ep, EPOLL_CTL_ADD, c->get_fd(), &ee) == -1) {
        Logger::instance()->warn("epoll ctl error, %d", errno);
        return false;
    }

    c->get_read_event()->set_active(true);
    c->get_write_event()->set_active(true);

    return true;
}

bool EpollModule::del_connection(Connection* c) {
    if (!c->is_close()) {
        // if fd is closed, it will be deleted by epoll
        // so we can not call epoll_ctl
        epoll_event ee;

        ee.events = 0;
        ee.data.ptr = NULL;

        if (epoll_ctl(ep, EPOLL_CTL_DEL, c->get_fd(), &ee) == -1) {
            return false;
        }
    }

    c->get_read_event()->set_active(false);
    c->get_write_event()->set_active(false);

    return true;
}

bool EpollModule::process_events() {
    int n = epoll_wait(ep, event_list, conf.epoll_events, -1);

    Logger::instance()->debug("epoll return, get %d", n);

    if (n == -1) {
        if (errno == EINTR) {
            if (sig_timer_alarm) {
                Clock::instance()->update();
                sig_timer_alarm = 0;
                return true;
            }
        }

        return false;
    }

    if (n == 0) {
        return false;
    }

    int flags;
    Event *event;
    Connection *c;

    for (int i = 0; i < n; ++i) {
        c = static_cast<Connection*>(event_list[i].data.ptr);

        if (c->is_close()) {    // Todo handle stale event
            Logger::instance()->info("connection already closed");
            continue;
        }

        flags = event_list[i].events;

        if (flags & (EPOLLERR | EPOLLHUP)) {
            flags |= (EPOLLIN | EPOLLOUT);
        }

        event = c->get_read_event();

        if ((flags & EPOLLIN) && event->is_active()) {
            Logger::instance()->debug("handle read event %p...", event);

            if (flags & EPOLLRDHUP) {
                // Todo
            }

            event->set_ready(true);
            event->handle();
        }

        event = c->get_write_event();

        if ((flags & EPOLLOUT) && event->is_active()) {
            Logger::instance()->debug("handle write event %p...", event);
            if (c->is_close()) {    // Todo handle stale event
                continue;
            }

            event->set_ready(true);
            event->handle();
        }
    }

    return true;
}

}
