#ifndef __CW_CLIENT_SOCKET_H_
#define __CW_CLIENT_SOCKET_H_

#include <iostream>
#include <vector>
#include <string_view>
#include <chrono>
#include <mutex>
#include "ListenSocket.h"
#include "Session.h"

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

    const Server* server;

    // TODO: implement timeout
    // std::chrono::steady_clock::time_point last_active;

    const static std::chrono::steady_clock::duration timeout;

    static const int MaxWriteSize;
    static size_t    socketCount;

    bool wantRead  = true;
    bool wantWrite = false;

    std::mutex                   mtx;
    std::unique_lock<std::mutex> lock;

    std::string ip;
    size_t      id;

    Session* currentSession = nullptr;

    std::string writeBuffer;

    static ClientSocket* from(ListenSocket* listenSocket, const Server* server, bool onePoll);

    int write(const char* data, size_t size, bool final, bool must = false, bool useCork = true);
    int write(const char* data, bool final = false);
    int write(const std::string& data, bool final = false);
    int write(const std::string_view& data, bool final = false);

    ClientSocket(SOCKET fd, const char* ip, const Server* server, bool oneShot);
    ClientSocket(const ClientSocket&) = delete;
    ClientSocket(ClientSocket&&)      = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;

    void loopPreCb();
    void loopPostCb();
    void onData(const std::string_view& data);
    void onWritable();
    void onAborted();
    void disconnect();

    ~ClientSocket();
};

}; // namespace cW

#endif