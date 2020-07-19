#ifndef __CW_HTTP_ROUTER_H_
#define __CW_HTTP_ROUTER_H_

#include <functional>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebSocket.h"

namespace cW {

typedef std::function<void(HttpRequest* req, HttpResponse*)> HttpHandler;
typedef std::function<void(WebSocket*)>                      WsHandler;

class Router {
    struct HttpRoute {
        HttpMethod  method;
        HttpHandler handler;
        UrlPath     path;
    };

    struct WsRoute {
        WsEvent   event;
        WsHandler handler;
        UrlPath   path;
    };

    std::vector<HttpRoute> httpRoutes;
    std::vector<WsRoute>   wsRoutes;

  public:
    void addHttpHandler(const char* route, HttpMethod method, HttpHandler&& handler);
    void addWsHandler(const char* route, WsEvent event, WsHandler&& handler);
    void dispatch(HttpRequest* request, HttpResponse* response) const;
    void dispatch(WsEvent event, WebSocket* ws) const;
    ~Router();
};

}; // namespace cW

#endif