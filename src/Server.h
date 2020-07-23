#ifndef __CW_SERVER_H_
#define __CW_SERVER_H_

#include <thread>
#include <initializer_list>
#include "Router.h"

namespace cW {

class ClientSocketSet;

class Server {
    friend class HttpSession;
    friend class WebSocketSession;

    Router                      router;
    const char*                 activeWsRoute = nullptr;
    std::vector<unsigned short> ports;

    inline void dispatch(HttpRequest* req, HttpResponse* res) const;
    inline void dispatch(WsEvent event, WebSocket* ws) const;

  public:
    Server();
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
    Server&& listen(std::initializer_list<short> ports);
    Server&& run(int nThreads = -1);
    ~Server();
};

void Server::dispatch(HttpRequest* req, HttpResponse* res) const { router.dispatch(req, res); }
void Server::dispatch(WsEvent event, WebSocket* ws) const { router.dispatch(event, ws); }

} // namespace cW

#endif