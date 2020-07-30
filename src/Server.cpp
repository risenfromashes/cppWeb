#include "Server.h"
#include "Poll.h"
#include "ListenSocket.h"
#include <iostream>
namespace cW {
Server::Server() {}

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
    activeWsRoute = route;
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

Server&& Server::listen(unsigned short port)
{
    ports.push_back(port);
    return std::move(*this);
}

Server&& Server::listen(std::initializer_list<short> ports)
{
    this->ports.insert(this->ports.end(), ports.begin(), ports.end());
    return std::move(*this);
}

Server&& Server::run(MTMode mtMode, int nThreads)
{
    if (nThreads < 0) nThreads = std::thread::hardware_concurrency();
    if (mtMode == ONE_LISTENER) {
        Poll poll(this, true);
        for (auto port : ports)
            poll.add(ListenSocket::create("::", port));
        poll.runLoop(nThreads);
    }
    else {
        std::vector<std::unique_ptr<std::thread>> threads;
        for (int i = 0; i < nThreads; i++)
            threads.push_back(std::make_unique<std::thread>([this] {
                Poll poll(this, false);
                for (auto port : ports) {
                    poll.add(ListenSocket::create("::", port, true));
                }
                poll.runLoop();
            }));
        for (int i = 0; i < nThreads; i++)
            if (threads[i]->joinable()) threads[i]->join();
    }
    return std::move(*this);
}

Server::~Server() {}
}; // namespace cW