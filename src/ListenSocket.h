#ifndef __CW_LISTEN_SOCKET_H_
#define __CW_LISTEN_SOCKET_H_

#include "Socket.h"

namespace cW {

struct ListenSocket : public Socket {
    friend class Poll;

    ListenSocket(const ListenSocket&) = delete;
    ListenSocket(ListenSocket&&)      = delete;
    ListenSocket& operator=(ListenSocket&) = delete;

    static ListenSocket* create(const char* host, int port, bool reuse_addr = false);

  private:
    ListenSocket(SOCKET fd, bool oneShot) : Socket(Type::LISTEN, fd, oneShot) {}

    static SOCKET createSocket(int family, int type, int protocol);
};
}; // namespace cW

#endif