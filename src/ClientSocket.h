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

class Server;

class ClientSocket : Socket {

    friend class Poll;

  protected:
    const Server* server;

    // TODO: implement timeout
    // std::chrono::steady_clock::time_point last_active;

    const static std::chrono::steady_clock::duration timeout;

    SOCKET fd;

    static const int MaxWriteSize;
    static size_t    socketCount;

    std::mutex                   mtx;
    std::unique_lock<std::mutex> lock;

    std::string ip;
    size_t      id;

    bool upgraded = false;
    // stores unwritten data
    std::string writeBuffer;
    std::string receiveBuffer;
    // actual writeOffset
    size_t writeOffset  = 0;
    size_t bufferOffset = 0;
    bool   lastWrite    = false;

    // move from another pointer
    ClientSocket(ClientSocket* socket);
    static ClientSocket* from(ListenSocket* listenSocket, const Server* server);

  public:
    ClientSocket(SOCKET fd, const char* ip, const Server* server);

    ClientSocket(const ClientSocket&) = delete;
    ClientSocket(ClientSocket&&)      = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;

    void disconnect();
    ~ClientSocket();

  protected:
    virtual void loopPreCb();
    virtual void loopPostCb();
    virtual void onData();
    virtual bool onWritable();
    virtual void onAborted();

  private:
    UpgradeSocket upgrade();
};

}; // namespace cW

#endif