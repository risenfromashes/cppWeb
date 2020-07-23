#ifndef __CW_CLIENT_SOCKET_H_
#define __CW_CLIENT_SOCKET_H_

#include <iostream>
#include <vector>
#include <string_view>
#include <chrono>
#include <mutex>
#include "ListenSocket.h"

namespace cW {
enum UpgradeSocket {
    DONT,
    HTTPSOCKET,
    WEBSOCKET
    // http2 or other protocols someday..
};

class Session;
class Server;

class ClientSocket : Socket {

    friend class Poll;
    friend class HttpSession;
    friend class WebSocketSession;

  protected:
    const Server* server;

    // TODO: implement timeout
    // std::chrono::steady_clock::time_point last_active;

    const static std::chrono::steady_clock::duration timeout;

    static const int MaxWriteSize;
    static size_t    socketCount;

    std::mutex                   mtx;
    std::unique_lock<std::mutex> lock;

    std::string ip;
    size_t      id;

    Session* currentSession = nullptr;

    /* writeBuffer is only to be used for must be written data that the socket owns*/
    std::string writeBuffer;
    std::string receiveBuffer;

    static ClientSocket* from(ListenSocket* listenSocket, const Server* server);

    int write(const char* data, size_t size, bool final);

    ClientSocket(SOCKET fd, const char* ip, const Server* server);

    ClientSocket(const ClientSocket&) = delete;
    ClientSocket(ClientSocket&&)      = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;

    ~ClientSocket();

    void loopPreCb();
    void loopPostCb();
    // poll for write?
    bool onData();
    // final?
    bool onWritable();
    void onAborted();
    void disconnect();
};

}; // namespace cW

#endif