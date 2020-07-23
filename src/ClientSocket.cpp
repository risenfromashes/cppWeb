#include "ClientSocket.h"
#include "Utils.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include "HttpSession.h"
#include "WebSocketSession.h"

namespace cW {

size_t    ClientSocket::socketCount  = 0;
const int ClientSocket::MaxWriteSize = 1024 * 1024;

const std::chrono::steady_clock::duration ClientSocket::timeout = std::chrono::seconds(5);

ClientSocket::ClientSocket(SOCKET fd, const char* ip, const Server* server)
    : Socket(Type::ACCEPT, fd), ip(ip), server(server)
{
}

ClientSocket* ClientSocket::from(ListenSocket* listenSocket, const Server* server)
{
    sockaddr_storage addr;
    uint32_t         addr_len = sizeof(addr);
    int              fd       = accept4( //
        listenSocket->fd,
        (sockaddr*)&addr,
        (socklen_t*)&addr_len,
        SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (fd < 0) {
        // perror("Failed to accept socket");
        return nullptr;
    }

    int enabled = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));

    char ip[64];
    // if (addr.ss_family == AF_INET6) {
    //     if (!inet_ntop(AF_INET6, (sockaddr_in6*)&addr, ip, 64))
    //         perror("Couldn't format ipv6 address");
    // }
    // else if (addr.ss_family == AF_INET) {
    if (!inet_ntop(AF_INET, (sockaddr_in*)&addr, ip, 64)) perror("Couldn't format ipv4 address");
    // }
    else
        strcpy(ip, "\0");
    return new ClientSocket(fd, ip, server);
}

//  first check if writebuffer is empty, if so write as much as possible, return bytes wrote
//  if the writeBuffer is not empty, try to write as much as possible
//  if writeBuffer is successfully emptied, write as much data as possible
int ClientSocket::write(const char* data, size_t size, bool final)
{
    static int disabled = 0;
    bool       msg_more = data && size > 0;
    int        ret;
    if (!writeBuffer.empty()) {
        ret = send(
            fd, writeBuffer.data(), (int)writeBuffer.size(), MSG_NOSIGNAL | (msg_more * MSG_MORE));
        if (ret < 0) {
            perror("Write error");
            return 0;
        }
        else {
            writeBuffer = writeBuffer.substr(ret);
            if (!writeBuffer.empty() || !msg_more) return ret;
        }
    }
    int bytesWrote = ret;
    msg_more       = !final;
    ret            = send(fd, data, size, MSG_NOSIGNAL | (msg_more * MSG_MORE));
    if (ret < 0)
        perror("Write error");
    else
        bytesWrote += ret;
    return bytesWrote;
}

ClientSocket::~ClientSocket()
{
    if (currentSession) delete currentSession;
}

void ClientSocket::loopPreCb()
{
    // lock.lock();
}
void ClientSocket::loopPostCb()
{
    if (currentSession) {
        if (currentSession->shouldEnd()) {
            delete currentSession;
            currentSession = nullptr;
        }
    }
    // last_active = std::chrono::steady_clock::now();
    // lock.unlock();
}
bool ClientSocket::onData()
{
    if (currentSession) {
        currentSession->onData();
        return true;
    }
    else {
        static const char* ws_match      = "upgrade: websocket";
        static int         ws_match_size = (int)strlen(ws_match);
        int                j             = 0;
        if (contains(receiveBuffer, "HTTP/") && contains(receiveBuffer, "\r\n\r\n")) {
            // naive linear string match since there's no repitition in match string
            for (size_t i = 0; i < receiveBuffer.size(); i++) {
                if (to_lower(receiveBuffer[i]) == ws_match[j]) {
                    j++;
                    if (j == ws_match_size) currentSession = new WebSocketSession(this);
                }
                else {
                    if (j != 0) i--; // match might start here
                    j = 0;
                }
            }
            currentSession = new HttpSession(this);
            return true;
        }
    }
    return false;
}
bool ClientSocket::onWritable()
{
    if (currentSession) return currentSession->onWritable();
    return true;
}
void ClientSocket::onAborted()
{
    if (currentSession) currentSession->onAborted();
    std::cout << "Socket " << id << " from " << ip << " aborted." << std::endl;
}

void ClientSocket::disconnect() { connected = false; }
}; // namespace cW