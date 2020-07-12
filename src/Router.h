#ifndef __CW_HTTP_ROUTER_H_
#define __CW_HTTP_ROUTER_H_

#include <functional>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebSocket.h"
#include "UrlParams.h"

namespace cW {
class WsSocket;

typedef std::function<void(HttpRequest* req, HttpResponse*)> HttpHandler;
typedef std::function<void(WebSocket*)>                      WsHandler;

class Router {
    struct HttpRoute {
        RE2         re_expression;
        HttpMethod  method;
        HttpHandler handler;
        UrlParams*  params;
        HttpRoute(const std::string& regexp,
                  HttpMethod         method,
                  HttpHandler&&      handler,
                  UrlParams*         params);
        ~HttpRoute();
    };

    struct WsRoute {
        RE2        re_expression;
        WsEvent    event;
        WsHandler  handler;
        UrlParams* params;
        WsRoute(const std::string& regexp, WsEvent event, WsHandler&& handler, UrlParams* params);
        ~WsRoute();
    };

    std::vector<HttpRoute*> httpRoutes;
    std::vector<WsRoute*>   wsRoutes;

    static UrlParams* parseUrlParams(std::string urlPattern);

  public:
    void addHttpHandler(const char* route, HttpMethod method, HttpHandler&& handler);
    void addWsHandler(std::string&& route, WsEvent event, WsHandler&& handler);
    void dispatch(HttpRequest* request, HttpResponse* response);
    void dispatch(WsEvent event, WebSocket* ws);
    ~Router();
};

}; // namespace cW

#endif