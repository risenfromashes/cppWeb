#include "HttpSocket.h"

namespace cW {
HttpSocket::HttpSocket(Socket* socket)
    : Socket(socket), request(new HttpRequest(receivedData)), response(new HttpResponse())
{
    server->dispatch(request, response);
}

void HttpSocket::onAwake()
{
    if (response->closeSocket) close();
}
void HttpSocket::onData()
{
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

void HttpSocket::onWritable()
{
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
        response->onWritableCallback(writeOffset);
        if (!response->writeBuffer.empty()) {
            size_t size = response->writeBuffer.size();
            writeOffset += write(response->writeBuffer.data(),
                                 (int)size,
                                 response->contentSize <= (size + writeOffset));
            response->writeBuffer = std::string_view(nullptr, 0);
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