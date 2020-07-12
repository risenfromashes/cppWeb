#include <iostream>
#include "Router.h"

namespace cW {

Router::HttpRoute::HttpRoute(const std::string& regexp,
                             HttpMethod         method,
                             HttpHandler&&      handler,
                             UrlParams*         params)
    : re_expression(regexp), method(method), handler(std::move(handler)), params(params)
{
}
Router::HttpRoute::~HttpRoute() { delete params; }

Router::WsRoute::WsRoute(const std::string& regexp,
                         WsEvent            event,
                         WsHandler&&        handler,
                         UrlParams*         params)
    : re_expression(regexp), event(event), handler(std::move(handler)), params(params)
{
}
Router::WsRoute::~WsRoute() { delete params; }

void Router::addHttpHandler(const char* route, HttpMethod method, HttpHandler&& handler)
{
    std::string urlPattern(route);
    UrlParams*  params = parseUrlParams(urlPattern);
    std::cout << "Adding url pattern: " << urlPattern << std::endl;
    httpRoutes.push_back(new HttpRoute(urlPattern, method, std::move(handler), params));
}
void Router::addWsHandler(std::string&& route, WsEvent event, WsHandler&& handler)
{
    std::string urlPattern(route);
    UrlParams*  params = parseUrlParams(urlPattern);
    wsRoutes.push_back(new WsRoute(urlPattern, event, std::move(handler), params));
}

void Router::dispatch(HttpRequest* request, HttpResponse* response)
{
    std::cout << "Router route size: " << httpRoutes.size() << std::endl
              << "First route method: " << httpRoutes[0]->method << std::endl;
    for (size_t i = 0; i < httpRoutes.size(); i++) {
        if (httpRoutes[i]->method == request->method &&
            ((httpRoutes[i]->params->count() == 0 &&
              RE2::FullMatch(request->absolutePath, httpRoutes[i]->re_expression)) ||
             (RE2::FullMatchN(request->absolutePath,
                              httpRoutes[i]->re_expression,
                              httpRoutes[i]->params->getArgs(),
                              httpRoutes[i]->params->count())))) {
            request->params = Params(*httpRoutes[i]->params);
            httpRoutes[i]->handler(request, response);
        }
    }
}

void Router::dispatch(WsEvent event, WebSocket* ws)
{
    for (size_t i = 0; i < httpRoutes.size(); i++) {
        if (wsRoutes[i]->event == event //
            && RE2::FullMatchN(ws->httpRequest->absolutePath,
                               wsRoutes[i]->re_expression,
                               wsRoutes[i]->params->getArgs(),
                               wsRoutes[i]->params->count())) {
            ws->httpRequest->params = Params(*httpRoutes[i]->params);
            wsRoutes[i]->handler(ws);
        }
    }
}

UrlParams* Router::parseUrlParams(std::string urlPattern)
{
    re2::StringPiece in(urlPattern);
    UrlParams*       params = new UrlParams();
    std::string      type, key;
    while (RE2::FindAndConsume(&in, UrlParams::re_urlParam, &type, &key)) {
        if (type == "d")
            params->add<int>(key);
        else if (type == "u")
            params->add<unsigned int>(key);
        else if (type == "ld")
            params->add<long int>(key);
        else if (type == "lu")
            params->add<unsigned long long>(key);
        else if (type == "lld")
            params->add<long long int>(key);
        else if (type == "llu")
            params->add<unsigned long long int>(key);
        else if (type == "f")
            params->add<float>(key);
        else if (type == "lf")
            params->add<double>(key);
        else if (type == "c")
            params->add<char>(key);
        else if (type == "s")
            params->add<std::string>(key);
        else
            throw std::exception("Unknown type specifier in url params.");
    }
    RE2::GlobalReplace(&urlPattern, "\\*", "[^/ ]+");
    RE2::GlobalReplace(&urlPattern, UrlParams::re_intParam, "([0-9]+)");
    RE2::GlobalReplace(&urlPattern, UrlParams::re_charParam, "([^/ ])");
    RE2::GlobalReplace(&urlPattern, UrlParams::re_strParam, "([^/ ]+)");
    RE2::GlobalReplace(&urlPattern, UrlParams::re_fracParam, "([0-9]*[.]*[0-9]+)");
    return params;
}

Router::~Router()
{
    for (auto route : httpRoutes)
        delete route;
    httpRoutes.clear();
    for (auto route : wsRoutes)
        delete route;
    wsRoutes.clear();
}

}; // namespace cW