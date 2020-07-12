#include "Server.h"
#include "SocketSet.h"

namespace cW {
Server::Server() { sockets = new SocketSet(); }

void Server::setActiveWsRoute(const char* route)
{
    auto size     = strlen(route);
    activeWsRoute = (char*)malloc(size);
    memcpy_s(activeWsRoute, size, route, size);
}

Server&& Server::get(const char* route, HttpHandler&& handler)
{
    router.addHttpHandler(route, HttpMethod::GET, std::move(handler));
    return std::move(*this);
}
Server&& Server::post(const char* route, HttpHandler&& handler)
{
    router.addHttpHandler(route, HttpMethod::POST, std::move(handler));
    return std::move(*this);
}
Server&& Server::put(const char* route, HttpHandler&& handler)
{
    router.addHttpHandler(route, HttpMethod::PUT, std::move(handler));
    return std::move(*this);
}
Server&& Server::del(const char* route, HttpHandler&& handler)
{
    router.addHttpHandler(route, HttpMethod::DEL, std::move(handler));
    return std::move(*this);
}
Server&& Server::head(const char* route, HttpHandler&& handler)
{
    router.addHttpHandler(route, HttpMethod::HEAD, std::move(handler));
    return std::move(*this);
}
Server&& Server::open(const char* route, WsHandler&& handler)
{
    setActiveWsRoute(route);
    router.addWsHandler(route, WsEvent::OPEN, std::move(handler));
    return std::move(*this);
}

Server&& Server::ping(WsHandler&& handler)
{
    assert(activeWsRoute && "No WsSocket route set.");
    router.addWsHandler(activeWsRoute, WsEvent::PING, std::move(handler));
    return std::move(*this);
}
Server&& Server::pong(WsHandler&& handler)
{
    assert(activeWsRoute && "No WsSocket route set.");
    router.addWsHandler(activeWsRoute, WsEvent::PONG, std::move(handler));
    return std::move(*this);
}
Server&& Server::close(WsHandler&& handler)
{
    assert(activeWsRoute && "No WsSocket route set.");
    router.addWsHandler(activeWsRoute, WsEvent::CLOSE, std::move(handler));
    return std::move(*this);
}

Server&& Server::message(WsHandler&& handler)
{
    assert(activeWsRoute && "No WsSocket route set.");
    router.addWsHandler(activeWsRoute, WsEvent::CLOSE, std::move(handler));
    return std::move(*this);
}

void Server::handleSocket(SOCKET socket, const std::string& ip) { sockets->add(socket, ip, this); }

Server&& Server::listen(unsigned short port)
{
    try {
        listeners.push_back(new TcpListener(port, this));
    }
    catch (TcpListenerError& error) {
        std::cout << error.what() << std::endl;
        std::cout << "Failed to listen on port " << port << std::endl;
    }
    return std::move(*this);
}

Server&& Server::run()
{
    static TIMEVAL timeout = {0, 100};
    while (true) {
        sockets->selectSockets();
        for (size_t i = 0; i < listeners.size(); i++)
            listeners[i]->listenOnce(&timeout);
    }
    return std::move(*this);
}

Server::~Server() { delete sockets; }
}; // namespace cW