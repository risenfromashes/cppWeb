#include <iostream>
#include "Router.h"

namespace cW {

void Router::addHttpHandler(const char* route, HttpMethod method, HttpHandler&& handler)
{
    httpRoutes.emplace_back(method, std::move(handler), UrlPath(route));
}
void Router::addWsHandler(const char* route, WsEvent event, WsHandler&& handler)
{
    wsRoutes.emplace_back(event, std::move(handler), UrlPath(route));
}

void Router::dispatch(HttpRequest* request, HttpResponse* response) const
{
    for (size_t i = 0; i < httpRoutes.size(); i++) {
        if (httpRoutes[i].method == request->method &&
            httpRoutes[i].path == request->absolutePath) {
            request->urlPath = &httpRoutes[i].path;
            httpRoutes[i].handler(request, response);
            return;
        }
    }
    response->end();
}

void Router::dispatch(WsEvent event, WebSocket* ws) const
{
    for (size_t i = 0; i < wsRoutes.size(); i++) {
        if (wsRoutes[i].event == event //
            && wsRoutes[i].path == ws->httpRequest->absolutePath) {
            ws->httpRequest->urlPath = &wsRoutes[i].path;
            wsRoutes[i].handler(ws);
            return;
        }
    }
}

Router::~Router() {}

}; // namespace cW