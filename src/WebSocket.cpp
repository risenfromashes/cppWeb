#include "WebSocket.h"

namespace cW {

WsMessage::WsMessage(WsOpcode opcode, const char* payload, size_t payloadSize)
    : opcode(opcode), payload(payload), payloadSize(payloadSize)
{
}

WebSocket::WebSocket(HttpRequest* request) : httpRequest(request) {}
WebSocket::~WebSocket()
{
    delete httpRequest;
    while (!queuedMessages.empty()) {
        delete queuedMessages.front();
        queuedMessages.pop();
    }
}

}; // namespace cW