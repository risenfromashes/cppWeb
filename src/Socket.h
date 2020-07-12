#ifndef __CW_SOCKET_H_
#define __CW_SOCKET_H_

// must include before winsock2
#include <re2/re2.h>

#include <WinSock2.h>
#include <string_view>
#include <iostream>
#include <vector>

namespace cW {

enum UpgradeSocket {
    HTTPSOCKET,
    WEBSOCKET
    // http2 or other protocols someday..
};

class Server;

class Socket {

    friend class SocketSet;
    friend class HttpSocket;
    friend class WsSocket;

  protected:
    Server* server;

    const static RE2 ws_upgrade_header_field_re;

    std::chrono::steady_clock::time_point last_active;

    const static std::chrono::steady_clock::duration timeout;

    SOCKET socket_handle;

    static const int writeSize;
    static size_t    socketCount;

    std::string ip;
    size_t      id;

    bool connected   = true;
    bool dataPending = true;

    bool upgraded = false;
    // stores unwritten data
    std::string writeBuffer;
    std::string receivedData;

    bool lastWriteWasSuccessful;

    // move from another pointer
    Socket(Socket* socket);

  public:
    Socket(SOCKET socket_handle, const std::string& ip, Server* server = nullptr);

    Socket(const Socket&) = delete;
    Socket(Socket&&)      = delete;
    Socket& operator=(const Socket&) = delete;

    void receive(char* receiveBuffer, int receiveBufferLength);

    bool hasDataPending();

    bool isConnected();

    // number of bytes written or added to write buffer
    // must not call with final set to true with size bigger than writeSize
    // it won't have any effect that way
    int write(const char* src, int size = 0, bool final = false);

    void setNoDelay(bool val);

    void close();

    ~Socket();

  protected:
    virtual void onAwake();
    virtual void onData();
    virtual void onWritable();
    virtual void onAborted();
    // upgrade when header is received
    virtual bool shouldUpgrade();

    UpgradeSocket upgrade();
};

}; // namespace cW

#endif