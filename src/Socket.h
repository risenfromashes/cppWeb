#ifndef __CW_SOCKET_H_
#define __CW_SOCKET_H_

#include <atomic>

#ifdef _GNU_SOURCE
typedef int SOCKET;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

namespace cW {

struct Socket {
    friend class Poll;
    enum Type { LISTEN, ACCEPT };

    const Type   type;
    const SOCKET fd;

    Socket(const Socket&) = delete;
    Socket(Socket&&)      = delete;
    Socket& operator=(Socket&) = delete;

  protected:
    std::atomic<bool> connected = true;
    void*             event;
    Socket(Type type, SOCKET fd);
    ~Socket();
};
}; // namespace cW
#endif