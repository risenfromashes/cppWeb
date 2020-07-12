#ifndef __CW_TCP_LISTENER_H_
#define __CW_TCP_LISTENER_H_
// to include this before winsock2
#include <re2/re2.h>
#include <iostream>
#include <exception>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <functional>

namespace cW {

class TcpListenerError : public std::runtime_error {

  private:
    int         error_code;
    const char* error_message;

  public:
    TcpListenerError(const char* message, int wsa_error_code);
    const char* what() const noexcept override;
};

class Server;

class TcpListener {

  private:
    // listen on both 127.0.0.1 and lan ip
    std::function<void(SOCKET, std::string)> handler = nullptr;

    char              hostname[80];
    SOCKET            local_host_socket, lan_socket;
    fd_set            fds;
    std::atomic<bool> shouldStop = false;

    Server* server = nullptr;

  public:
    TcpListener(unsigned short port);
    TcpListener(unsigned short port, Server* server);

    void listenOnce(TIMEVAL* timeout);

    void close();
    ~TcpListener();

  protected:
    void socketHandler(SOCKET Socket, std::string ip);

  private:
    void acceptSocket(const SOCKET& socket_handle);

    static char* createListenSocket(SOCKET&     socket_handle,
                                    const char* host,
                                    const char* port,
                                    addrinfo&   hints,
                                    addrinfo*&  addr_info,
                                    const char* name);

    static void cleanUp(addrinfo*& addr_info, SOCKET& socket_handle);
};
} // namespace cW

#endif