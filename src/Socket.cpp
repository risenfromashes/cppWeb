#include "Socket.h"
#include "Utils.h"
namespace cW {

size_t    Socket::socketCount = 0;
const int Socket::writeSize   = 512 * 1024;

const RE2 Socket::ws_upgrade_header_field_re("(?i)\r\nUpgrade\\s*:\\s*websocket\\s*\r\n");

const std::chrono::steady_clock::duration Socket::timeout = std::chrono::seconds(5);

Socket::Socket(Socket* socket)
    : socket_handle(socket->socket_handle),
      server(socket->server),
      ip(socket->ip),
      id(socket->id),
      writeBuffer(std::move(socket->writeBuffer)),
      receivedData(std::move(socket->receivedData)),
      lastWriteWasSuccessful(socket->lastWriteWasSuccessful)
{
    // cause the other socket will try to close on destruction
    socket->connected = false;
    delete socket;
    last_active = std::chrono::steady_clock::now();
}

Socket::Socket(SOCKET socket_handle, const std::string& ip, Server* server)
    : socket_handle(socket_handle), ip(ip), server(server)
{
    id = socketCount++;

    static unsigned long enabled = 1;
    // make non-blocking
    ioctlsocket(socket_handle, FIONBIO, &enabled);

    std::cout << "Socket created with id " << id << " from ip " << ip << "." << std::endl;
    last_active = std::chrono::steady_clock::now();
}

void Socket::receive(char* receiveBuffer, int receiveBufferLength)
{
    last_active = std::chrono::steady_clock::now();
    int result  = recv(socket_handle, receiveBuffer, receiveBufferLength, 0);
    if (result > 0) {
        std::cout << "Received on socket " << id << " " << result << std::endl;
        receivedData.append(receiveBuffer, result);
        onData();
        dataPending = false;
    }
    else if (result == 0) {
        dataPending = false;
    }
    else {
        int error_code = WSAGetLastError();
        if (error_code == WSAEMSGSIZE) {
            receivedData.append(receiveBuffer, receiveBufferLength);
            onData();
            dataPending = true;
        }
        else if (error_code == WSAEWOULDBLOCK) {
            dataPending = false;
        }
        else {
            switch (error_code) {
                case WSAESHUTDOWN:
                case WSAEHOSTUNREACH:
                case WSAECONNABORTED:
                case WSAECONNRESET:
                case WSAETIMEDOUT:
                    std::cout << "Socket " << id << " from ip " << ip << "failed with error "
                              << error_code << std::endl;
                    connected = false;
                    onAborted();
                    break;
                default:
                    std::cout << "Receive on socket " << id << " from ip " << ip
                              << "failed with error " << error_code << std::endl;
                    break;
            }
        }
    }
}

bool Socket::hasDataPending() { return dataPending; }

bool Socket::isConnected() { return connected; }

int Socket::write(const char* src, int size, bool final)
{
    last_active = std::chrono::steady_clock::now();
    if (size == 0) size = (int)strlen(src);
    int result;
    int writeBufferSize = (int)writeBuffer.size();
    int toAdd           = min(writeSize - writeBufferSize, size);
    writeBuffer.append(src, toAdd);
    writeBufferSize += toAdd;
    char* buffer = writeBuffer.data();
    // write only in parts of 512kb in hopes of avoiding the 200ms naggle delay
    if (writeBufferSize >= writeSize || final) {
        if (final) setNoDelay(true);
        result = send(socket_handle, buffer, writeBufferSize, 0);
        if (result == SOCKET_ERROR) {
            int error_code = WSAGetLastError();
            switch (error_code) {
                case WSAEWOULDBLOCK:
                    // TODO: make error proof
                    return toAdd;
                    break;
                case WSAESHUTDOWN:
                case WSAEHOSTUNREACH:
                case WSAECONNABORTED:
                case WSAECONNRESET:
                case WSAETIMEDOUT:
                    std::cout << "Socket " << id << " from ip " << ip << " disconnected with error "
                              << error_code << std::endl;
                    connected = false;
                    onAborted();
                    break;
                default:
                    std::cout << "Write on socket " << id << " from ip " << ip
                              << "failed with error " << error_code << std::endl;
                    break;
            }
        }
        else {
            // queue buffer for next write
            writeBuffer = writeBuffer.substr(result);
        }
        lastWriteWasSuccessful = result == writeBufferSize;
        if (final) setNoDelay(false);
    }
    return toAdd;
}

void Socket::setNoDelay(bool val)
{
    int state = (int)val;
    setsockopt(socket_handle, IPPROTO_TCP, TCP_NODELAY, (char*)&state, sizeof(state));
}

void Socket::close()
{
    std::cout << "Trying to close socket." << std::endl;
    if (shutdown(socket_handle, SD_SEND) == SOCKET_ERROR) {
        std::cout << "Failed to close socket " << id << " from " << ip << ". Error "
                  << WSAGetLastError() << std::endl;
    }
    else {
        Clock::printElapsed("Closed socket " + std::to_string(id) + " from " + ip + ".");
    }
    connected = false;
}

Socket::~Socket()
{
    if (connected) close();
}

void Socket::onAwake() {}
void Socket::onData() {}
void Socket::onWritable() {}
void Socket::onAborted()
{
    std::cout << "Socket " << id << " from " << ip << " aborted." << std::endl;
}
// upgrade when header is received
bool Socket::shouldUpgrade() { return receivedData.find("\r\n\r\n") != std::string::npos; }

UpgradeSocket Socket::upgrade()
{
    Clock::printElapsed("Upgrading socket.");
    static const char* match     = "upgrade: websocket";
    static int         matchSize = (int)strlen(match);
    int                j         = 0;
    for (size_t i = 0; i < receivedData.size(); i++) {
        if ((char)tolower(receivedData[i]) == match[j]) {
            j++;
            if (j == matchSize) return UpgradeSocket::WEBSOCKET;
        }
        else {
            if (j != 0) i--;
            j = 0;
        }
    }
    return UpgradeSocket::HTTPSOCKET;
}

}; // namespace cW