#include "connection.h"

namespace servx {

Connection::Connection()
    : fd(-1),
      read_event(new Event(this, 0)),
      write_event(new Event(this, 1)) {
}

}
