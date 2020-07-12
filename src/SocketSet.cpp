#include "SocketSet.h"

namespace cW {
void SocketSet::add(Socket* socket)
{

    std::scoped_lock lock(socketSetLock);
    sockets.push_back(socket);
}

SocketSet::SocketSet() { receiveBuffer = (char*)malloc(sizeof(char) * receiveBufferSize); }

void SocketSet::selectSockets()
{
    std::scoped_lock lock(socketSetLock);
    if (!sockets.empty()) {
        for (int i = 0; i < sockets.size(); i++) {
            if (!sockets[i]->connected || (sockets[i]->timeout + sockets[i]->last_active <=
                                           std::chrono::steady_clock::now())) {
                delete sockets[i];
                sockets.erase(sockets.begin() + i);
                if (sockets.empty()) return;
                continue;
            }
        }
        for (int i = 0; i < sockets.size(); i++) {
            // Clock::printElapsed("Running socket read/write loop. " +
            //                     std::to_string(sockets.size()));
            sockets[i]->onAwake();
            sockets[i]->receive(receiveBuffer, receiveBufferSize);
            sockets[i]->onWritable();

            // if (sockets[i]->shouldUpgrade()) {
            //     switch (sockets[i]->upgrade()) {
            //         case UpgradeSocket::HTTPSOCKET: sockets[i] = new HttpSocket(sockets[i]);
            //         break; case UpgradeSocket::WEBSOCKET: sockets[i] = new WsSocket(sockets[i]);
            //         break; default: break;
            //     }
            // }
            sockets[i]->onAwake();
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