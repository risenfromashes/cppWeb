#ifndef __CW_SERVER_H_
#define __CW_SERVER_H_

#include <thread>
#include "TcpListener.h"
#include "Router.h"

namespace cW {

class SocketSet;

class Server {
    friend class HttpSocket;
    friend class WsSocket;

    Router router;
    char*  activeWsRoute = nullptr;

    SocketSet* sockets;

    std::vector<TcpListener*> listeners;

    std::thread listenerThread;

    void setActiveWsRoute(const char* route);

    inline void dispatch(HttpRequest* req, HttpResponse* res);
    inline void dispatch(WsEvent event, WebSocket* ws);

  public:
    Server();
    void handleSocket(SOCKET socket, const std::string& ip);

    Server&& get(const char* route, HttpHandler&& handler);
    Server&& post(const char* route, HttpHandler&& handler);
    Server&& put(const char* route, HttpHandler&& handler);
    Server&& del(const char* route, HttpHandler&& handler);
    Server&& head(const char* route, HttpHandler&& handler);
    Server&& open(const char* route, WsHandler&& handler);
    Server&& ping(WsHandler&& handler);
    Server&& pong(WsHandler&& handler);
    Server&& close(WsHandler&& handler);
    Server&& message(WsHandler&& handler);
    Server&& listen(unsigned short port);
    Server&& run();
    ~Server();
};

void Server::dispatch(HttpRequest* req, HttpResponse* res) { router.dispatch(req, res); }
void Server::dispatch(WsEvent event, WebSocket* ws) { router.dispatch(event, ws); }

} // namespace cW

#endif