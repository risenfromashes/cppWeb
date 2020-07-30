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

ClientSocket::ClientSocket(SOCKET fd, const char* ip, const Server* server, bool oneShot)
    : Socket(Type::ACCEPT, fd, oneShot), ip(ip), server(server)
{
}

ClientSocket* ClientSocket::from(ListenSocket* listenSocket, const Server* server, bool onePoll)
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
    if (addr.ss_family == AF_INET6) {
        if (!inet_ntop(AF_INET6, (sockaddr_in6*)&addr, ip, 64))
            perror("Couldn't format ipv6 address");
    }
    else if (addr.ss_family == AF_INET) {
        if (!inet_ntop(AF_INET, (sockaddr_in*)&addr, ip, 64))
            perror("Couldn't format ipv4 address");
    }
    else
        strcpy(ip, "\0");
    return new ClientSocket(fd, ip, server, onePoll);
}

//  first check if writebuffer is empty, if so write as much as possible, return bytes wrote
//  if the writeBuffer is not empty, try to write  as much as possible
//  if writeBuffer is successfully emptied, write as much data as possible
int ClientSocket::write(const char* data, size_t size, bool final, bool must, bool useCork)
{
    static const unsigned int         corkBufferLength = 512 * 1024;
    static thread_local char* const   corkBuffer       = (char*)malloc(corkBufferLength);
    static thread_local int           corkBufferOffset = 0;
    static thread_local ClientSocket* corkedSocket     = nullptr;

    bool        msg_more, fromWriteBuffer = !writeBuffer.empty();
    const char* buf;
    int         bufLen;
    if (fromWriteBuffer) {
        msg_more = data && size > 0;
        buf      = writeBuffer.data();
        bufLen   = writeBuffer.size();
    }
    else {
        if (useCork) {
            if (corkedSocket == this || (corkedSocket == nullptr && !final)) {
                corkedSocket = this;
                bool writtenToCork;
                if (writtenToCork = (corkBufferOffset + size <= corkBufferLength)) {
                    std::memcpy(corkBuffer + corkBufferOffset, data, size);
                    corkBufferOffset += size;
                    if (!final) return size;
                }
                // uncork
                int ret =
                    (corkBufferOffset == 0 ||
                     corkBufferOffset ==
                         write(
                             corkBuffer, corkBufferOffset, writtenToCork && final, true, false)) &&
                            !writtenToCork
                        ? write(data, size, final, must, false)
                        : (writtenToCork ? size : 0);
                corkedSocket     = nullptr;
                corkBufferOffset = 0;
                return ret;
            }
        }
        msg_more = !final;
        buf      = data;
        bufLen   = size;
    }
    int wrote = send(fd, buf, bufLen, MSG_NOSIGNAL | (msg_more * MSG_MORE));
    if (wrote < 0) {
        perror("Write error");
        wrote = 0;
    }
    wantWrite = !writeBuffer.empty() || msg_more || wrote < bufLen;
    if (fromWriteBuffer) {
        writeBuffer = writeBuffer.substr(wrote);
        if (msg_more) {
            if (writeBuffer.empty())
                return write(data, size, final, must, useCork);
            else if (must)
                writeBuffer.append(data, size);
        }
        return 0;
    }
    else if (must && wrote < size)
        writeBuffer.append(data + wrote, size - wrote);
    return wrote;
}

int ClientSocket::write(const char* data, bool final)
{
    return write(data, strlen(data), final, true);
}
int ClientSocket::write(const std::string& data, bool final)
{
    return write(data.data(), data.size(), final, true);
}

int ClientSocket::write(const std::string_view& data, bool final)
{
    return write(data.data(), data.size(), final, true);
}

ClientSocket::~ClientSocket()
{
    if (currentSession) delete currentSession;
}

void ClientSocket::loopPreCb()
{
    if (currentSession) { currentSession->onAwakePre(); }
    // lock.lock();
}
void ClientSocket::loopPostCb()
{
    if (currentSession) {
        if (currentSession->shouldEnd()) {
            delete currentSession;
            currentSession = nullptr;
            writeBuffer.clear();
        }
        else
            currentSession->onAwakePost();
    }
    // last_active = std::chrono::steady_clock::now();
    // lock.unlock();
}

void ClientSocket::onData(const std::string_view& data)
{
    if (currentSession)
        currentSession->onData(data);
    else {
        static const char* ws_match      = "upgrade: websocket";
        static int         ws_match_size = (int)strlen(ws_match);
        int                j             = 0;
        size_t             headerEnd     = data.find("\r\n\r\n");
        if (contains(data, "HTTP/") && headerEnd != std::string_view::npos) {
            // naive linear string match since there's no repitition in match string
            for (size_t i = 0; i < data.size(); i++) {
                if (to_lower(data[i]) == ws_match[j]) {
                    j++;
                    if (j == ws_match_size) {
                        currentSession = new WebSocketSession(this);
                        wantWrite      = true;
                        return;
                    }
                }
                else {
                    if (j != 0) i--; // match might start here
                    j = 0;
                }
            }
            currentSession = new HttpSession(this, data.substr(0, headerEnd + 2));
            if (data.size() > headerEnd + 4) currentSession->onData(data.substr(headerEnd + 4));
            wantWrite = true;
        }
    }
}
// final?
void ClientSocket::onWritable()
{
    if (currentSession)
        currentSession->onWritable();
    else
        wantWrite = false;
}
void ClientSocket::onAborted()
{
    if (currentSession) currentSession->onAborted();
    std::cout << "Socket " << id << " from " << ip << " aborted." << std::endl;
}

void ClientSocket::disconnect() { connected = false; }
}; // namespace cW