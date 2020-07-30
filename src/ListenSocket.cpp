#include "ListenSocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>

namespace cW {

ListenSocket* ListenSocket::create(const char* host, int port, bool reuse_addr)
{
    addrinfo hints, *addr_result;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family   = AF_UNSPEC;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    char port_str[16];
    snprintf(port_str, 16, "%d", port);
    if (getaddrinfo(host, port_str, &hints, &addr_result)) {
        perror("Couldn't retrieve host addresses");
        return nullptr;
    }
    SOCKET    listenSocket = INVALID_SOCKET;
    addrinfo* listenAddr;
    // prefer ipv6
    for (auto addr = addr_result; addr && listenSocket == INVALID_SOCKET; addr = addr->ai_next) {
        if (addr->ai_family == AF_INET6) {
            listenSocket = createSocket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            listenAddr   = addr;
        }
    }

    for (auto addr = addr_result; addr && listenSocket == INVALID_SOCKET; addr = addr->ai_next) {
        if (addr->ai_family == AF_INET) {
            listenSocket = createSocket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            listenAddr   = addr;
        }
    }
    if (reuse_addr) {
        int enabled = 1;
        setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enabled, sizeof(enabled));
        setsockopt(listenSocket, SOL_SOCKET, SO_REUSEPORT, (const char*)&enabled, sizeof(enabled));
    }
    int disabled = 0;
    setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&disabled, sizeof(disabled));

    if (bind(listenSocket, listenAddr->ai_addr, (socklen_t)listenAddr->ai_addrlen) < 0) {
        perror("Binding failed");
        return nullptr;
    }
    if (listen(listenSocket, 512) < 0) {
        perror("Listening error");
        return nullptr;
    }
    return new ListenSocket(listenSocket, !reuse_addr);
}

SOCKET ListenSocket::createSocket(int family, int type, int protocol)
{
    int flags = SOCK_CLOEXEC | SOCK_NONBLOCK;
    return socket(family, type | flags, protocol);
}

}; // namespace cW