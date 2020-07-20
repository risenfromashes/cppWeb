#include "Socket.h"
#include <sys/epoll.h>
#include <cstdlib>

namespace cW {

typedef epoll_event EVENT;

Socket::Socket(Type type, SOCKET fd, uint32_t events) : type(type), fd(fd)
{
    auto _event      = (EVENT*)malloc(sizeof(EVENT));
    _event->data.ptr = this;
    // even for accept sockets don't write if there is no incoming data
    _event->events = EPOLLIN
                     //|EPOLLONESHOT
                     | events;
    event = _event;
}

Socket::~Socket() { free(event); }

} // namespace cW
