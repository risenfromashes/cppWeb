#include "HttpSocket.h"

namespace cW {
HttpSocket::HttpSocket(Socket* socket)
    : Socket(socket), request(new HttpRequest(receivedData)), response(new HttpResponse())
{
    Clock::printElapsed("Dispatching.");
    upgraded = true;
    server->dispatch(request, response);
}

HttpSocket::HttpSocket(SOCKET socket_handle, const std::string& ip, Server* server)
    : Socket(socket_handle, ip, server)
{
}

void HttpSocket::onAwake()
{
    if (gotHeader && response->closeSocket) { close(); }
}
void HttpSocket::onData()
{
    if (!gotHeader) {
        if (receivedData.find("\r\n\r\n") != std::string::npos) {
            gotHeader = true;
            request   = new HttpRequest(receivedData);
            response  = new HttpResponse();
            Clock::printElapsed("Dispatching.");
            server->dispatch(request, response);
        }
    }
    else {
        if (request->onDataCallback)
            request->onDataCallback(std::string_view(receivedData));
        else if (request->onBodyCallback) {
            if (!dataPending)
                request->onBodyCallback(std::string_view(receivedData));
            else
                return;
        }
        receivedData.clear();
    }
}

void HttpSocket::onWritable()
{
    if (gotHeader) {
        if (!wroteHeader) {
            std::string header;
            header += ("HTTP/1.1 " + HttpStatus::status(response->statusCode) + "\r\n");
            for (auto [name, value] : response->headers)
                header += (name + ": " + value + "\r\n");
            header += "\r\n";
            write(header.c_str(), (int)header.size());
            wroteHeader = true;
        }
        if (response->onWritableCallback) {
            std::cout << "Response: " << response << std::endl;
            response->onWritableCallback(writeOffset);
            // std::cout << "Buffer size: " << response->buffer.size() << std::endl;
            if (!response->buffer.empty()) {
                std::cout << "Write callback" << std::endl;
                size_t size  = response->buffer.size();
                bool   final = response->contentSize <= (size + writeOffset);
                int    wrote = write(response->buffer.data(), (int)size, final);
                Clock::printElapsed("Wrote " + std::to_string(wrote));
                if (final && size == wrote) close();
                writeOffset += wrote;
                response->buffer = std::string_view(nullptr, 0);
            }
        }
    }
}
bool HttpSocket::shouldUpgrade() { return false; }

HttpSocket::~HttpSocket()
{
    delete response;
    delete request;
}

} // namespace cW