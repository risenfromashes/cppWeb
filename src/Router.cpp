#include <iostream>
#include "Router.h"

namespace cW {

void Router::addHttpHandler(const char* route, HttpMethod method, HttpHandler&& handler)
{
    httpRoutes.push_back(
        new HttpRoute{.method = method, .handler = handler, .path = UrlPath(route)});
}
void Router::addWsHandler(const char* route, WsEvent event, WsHandler&& handler)
{
    wsRoutes.push_back(new WsRoute{.event = event, .handler = handler, .path = UrlPath(route)});
}

bool Router::dispatch(HttpRequest* request, HttpResponse* response) const
{
    // printf("Routing...\n");
    for (size_t i = 0; i < httpRoutes.size(); i++) {
        if (httpRoutes[i]->method == request->method &&
            httpRoutes[i]->path == request->absolutePath) {
            request->urlPath = &(httpRoutes[i]->path);
            httpRoutes[i]->handler(request, response);
            return true;
        }
    }
    return false;
}

bool Router::dispatch(WsEvent event, WebSocket* ws) const
{
    for (size_t i = 0; i < wsRoutes.size(); i++) {
        if (wsRoutes[i]->event == event //
            && wsRoutes[i]->path == ws->httpRequest->absolutePath) {
            ws->httpRequest->urlPath = &(wsRoutes[i]->path);
            wsRoutes[i]->handler(ws);
            return true;
        }
    }
    return false;
}

Router::~Router()
{
    for (auto route : httpRoutes)
        delete route;
    for (auto route : wsRoutes)
        delete route;
    httpRoutes.clear();
    wsRoutes.clear();
}

}; // namespace cW