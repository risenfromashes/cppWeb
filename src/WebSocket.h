#ifndef __CW_WEB_SOCKET_H_
#define __CW_WEB_SOCKET_H_

#include <queue>
#include "HttpRequest.h"
namespace cW {

enum WsEvent { OPEN, MESSAGE, PING, PONG, CLOSE, UPGRADE };

enum WsOpcode { Continuation = 0, Text = 1, Binary = 2, Close = 8, Ping = 9, Pong = 10 };

struct WsMessage {
    friend class WsSocket;
    friend class WebSocket;

    const WsOpcode opcode;

    inline std::string_view data() { return std::string_view(payload, payloadSize); }

  private:
    // doesn't own payload
    WsMessage(WsOpcode opcode, const char* payload, size_t payloadSize);
    const char* payload;
    size_t      payloadSize;
};

// the request+response of websocket
class WebSocket {
    friend class WsSocket;
    friend class Router;

    HttpRequest* httpRequest;

    WsMessage* currentMessage;

    // outgoing message queue
    std::queue<WsMessage*> queuedMessages;

    WebSocket(HttpRequest* request);

  public:
    template <typename T>
    T getRequestParam(const std::string& key);

    inline std::string_view getRequestHeader(const std::string& name);
    inline std::string_view getRequestQuery(const std::string& query);
    inline const WsMessage& getMessage();
    inline void             sendMessage(WsOpcode opcode, const char* data, size_t size);

    ~WebSocket();
};

template <typename T>
T WebSocket::getRequestParam(const std::string& key)
{
    return httpRequest->getParam<T>(key);
}
std::string_view WebSocket::getRequestHeader(const std::string& name)
{
    return httpRequest->getHeader(name);
}
std::string_view WebSocket::getRequestQuery(const std::string& query)
{
    httpRequest->getQuery(query);
}

const WsMessage& WebSocket::getMessage() { return *currentMessage; }
void             WebSocket::sendMessage(WsOpcode opcode, const char* data, size_t size)
{
    if (!size) size = strlen(data);
    queuedMessages.push(new WsMessage(opcode, data, size));
}

}; // namespace cW

#endif