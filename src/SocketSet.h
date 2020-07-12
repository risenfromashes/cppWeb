#ifndef __CW_SOCKET_SET_H_
#define __CW_SOCKET_SET_H_

#include "HttpSocket.h"
#include "WsSocket.h"

namespace cW {
// a number of client sockets to be managed by the same thread
class SocketSet {
    friend class Server;

    fd_set               writeFds, readFds;
    std::vector<Socket*> sockets;

    char*            receiveBuffer;
    static const int receiveBufferSize = 512 * 1024;

    void add(Socket* socket);
    void add(SOCKET socket, const std::string& ip, Server* server);

    SocketSet();

    void selectSockets();
    ~SocketSet();
};
} // namespace cW

#endif