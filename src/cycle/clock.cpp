#include "clock.h"

#include <sys/time.h>

#include "logger.h"

namespace servx {

const char* Clock::week[] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

const char* Clock::months[] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

Clock* Clock::clock = new Clock;

Clock::Clock(): cur(0) {
    log_time[0] = "1970/01/01 00:00:00";
    log_time[1] = "1970/01/01 00:00:00";
    http_time[0] = "Thu, 01 Jan 1970 00:00:00 GMT";
    http_time[1] = "Thu, 01 Jan 1970 00:00:00 GMT";
}

void Clock::update() {
    if (!mtx.try_lock()) {
        return;
    }

    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        Logger::instance()->warn("invoke gettimeofday() error");
        return;
    }

    uint64_t now = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if (now - get_current_milliseconds() < 1000) {
        milliseconds[cur] = now;
        return;
    }

    struct tm t;
    if (gmtime_r(&tv.tv_sec, &t) == NULL) {
        Logger::instance()->warn("invoke gmtime_r() error");
        return;
    }

    milliseconds[!cur] = now;

    char *s1 = const_cast<char*>(log_time[!cur].c_str());
    char *s2 = const_cast<char*>(http_time[!cur].c_str());

    sprintf(s1, "%4d/%02d/%02d %02d:%02d:%02d",
        t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    sprintf(s2, "%s, %02d %s %4d %02d:%02d:%02d GMT",
        week[t.tm_wday], t.tm_mday, months[t.tm_mon - 1], t.tm_year,
        t.tm_hour, t.tm_min, t.tm_sec);

    cur = !cur;

    mtx.unlock();
}

int Clock::format_http_time(time_t sec, char* buf) {
    struct tm t;
    if (gmtime_r(&sec, &t) == NULL) {
        return false;
    }

    return sprintf(buf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
        week[t.tm_wday], t.tm_mday, months[t.tm_mon - 1], t.tm_year,
        t.tm_hour, t.tm_min, t.tm_sec);
}

}
