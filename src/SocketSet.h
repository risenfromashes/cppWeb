#ifndef __CW_SOCKET_SET_H_
#define __CW_SOCKET_SET_H_

#include <mutex>
#include "HttpSocket.h"
#include "WsSocket.h"
namespace cW {
// a number of client sockets to be managed by the same thread
class SocketSet {
    friend class Server;

    fd_set               writeFds, readFds;
    std::vector<Socket*> sockets;

    std::mutex socketSetLock;

    char*            receiveBuffer;
    static const int receiveBufferSize = 512 * 1024;

    void add(Socket* socket);

    SocketSet();

    void selectSockets();
    ~SocketSet();
};
} // namespace cW

#endif