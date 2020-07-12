#include "SocketSet.h"

namespace cW {
void SocketSet::add(Socket* socket) { sockets.push_back(socket); }
void SocketSet::add(SOCKET socket, const std::string& ip, Server* server)
{
    sockets.push_back(new Socket(socket, ip, server));
}

SocketSet::SocketSet() { receiveBuffer = (char*)malloc(sizeof(char) * receiveBufferSize); }

void SocketSet::selectSockets()
{
    // timeout of maximum 100us
    static TIMEVAL* timeout = new TIMEVAL{0, 100};
    if (!sockets.empty()) {
        FD_ZERO(&writeFds);
        FD_ZERO(&readFds);

        for (int i = 0; i < sockets.size(); i++) {
            FD_SET(sockets[i]->socket_handle, &readFds);
            FD_SET(sockets[i]->socket_handle, &writeFds);
        }
        int result = select(0, &readFds, &writeFds, nullptr, timeout);
        if (result == SOCKET_ERROR) {
            std::cout << "Selecting in SocketSet failed with error " << WSAGetLastError() << "."
                      << std::endl;
        }
        else if (result > 0) {
            for (int i = 0; i < sockets.size(); i++) {
                sockets[i]->onAwake();
                if (FD_ISSET(sockets[i]->socket_handle, &readFds)) {
                    sockets[i]->receive(receiveBuffer, receiveBufferSize);
                }
                // if there is left-over data, call onData
                else if (!sockets[i]->receivedData.empty())
                    sockets[i]->onData();
                if (FD_ISSET(sockets[i]->socket_handle, &writeFds)) sockets[i]->onWritable();
                if (sockets[i]->shouldUpgrade()) {
                    switch (sockets[i]->upgrade()) {
                        case UpgradeSocket::HTTPSOCKET:
                            sockets[i] = new HttpSocket(sockets[i]);
                            break;
                        case UpgradeSocket::WEBSOCKET: sockets[i] = new WsSocket(sockets[i]); break;
                        default: break;
                    }
                }
            }
            for (int i = 0; i < sockets.size(); i++) {
                if (!sockets[i]->connected) {
                    delete sockets[i];
                    sockets.erase(sockets.begin() + i);
                }
            }
        }
    }
}
SocketSet::~SocketSet()
{
    for (auto socket : sockets)
        delete socket;
    sockets.clear();
}
}; // namespace cW