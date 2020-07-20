#include "HttpSocket.h"
#include "Server.h"

namespace cW {
HttpSocket::HttpSocket(ClientSocket* other) : ClientSocket(other)
{
    upgraded = true;
    dispatch();
}

void HttpSocket::dispatch()
{
    auto headerEnd = receiveBuffer.find("\r\n\r\n");
    requestHeader  = receiveBuffer.substr(0, headerEnd + 2);
    receiveBuffer  = receiveBuffer.substr(headerEnd + 4);
    request        = new HttpRequest(requestHeader);
    response       = new HttpResponse();
    // Clock::printElapsed("Dispatching.");
    server->dispatch(request, response);
}

void HttpSocket::badRequest()
{
    writeBuffer += ("HTTP/1.1 " + HttpStatus::status(HttpStatus::BadRequest) + "\r\n\r\n");
    write(nullptr, 0, true);
    response->close = true;
}

void HttpSocket::loopPreCb()
{
    ClientSocket::loopPreCb();
    if (writeBuffer.empty() &&
        (response->close || writeOffset - responseHeaderLength >= response->contentSize)) {
        disconnect();
    }
}
void HttpSocket::loopPostCb()
{
    if (writeBuffer.empty() &&
        (response->close || writeOffset - responseHeaderLength >= response->contentSize)) {
        disconnect();
    }
    ClientSocket::loopPostCb();
}
void HttpSocket::onData()
{
    try {
        if (request->onDataCallback) {
            request->onDataCallback(receiveBuffer);
            receiveBuffer.clear();
        }
        else if (request->onBodyCallback) {
            if (requestContentLength < 0)
                requestContentLength = request->getHeader<int64_t>("Content-Length");
            if (requestContentLength <= receiveBuffer.size()) {
                request->onBodyCallback(receiveBuffer);
                receiveBuffer.clear();
            }
        }
    }
    catch (std::runtime_error& error) {
        std::cerr << error.what() << std::endl;
        if (!writing) badRequest();
    }
}

void HttpSocket::onWritable()
{
    if (!wroteHeader) {
        assert(writeBuffer.size() == 0 && "How is size not zero here?");
        writeBuffer += ("HTTP/1.1 " + HttpStatus::status(response->statusCode) + "\r\n");
        for (auto [name, value] : response->headers)
            writeBuffer += (name + ": " + value + "\r\n");
        writeBuffer += "\r\n";
        wroteHeader          = true;
        responseHeaderLength = writeBuffer.size();
    }
    // drain

    if (response->onWritableCallback) {
        try {
            response->onWritableCallback(std::min<int64_t>(writeOffset - responseHeaderLength, 0));
        }
        catch (std::runtime_error& error) {
            std::cerr << error.what() << std::endl;
            if (!writing) badRequest();
            response->close = true;
            return;
        }
    }
    if (!response->buffer.empty()) {
        writing             = true;
        size_t bufferLength = response->buffer.length();
        bool   final = (writeOffset + bufferLength - responseHeaderLength) >= response->contentSize;
        int    wrote = write(response->buffer.data(), bufferLength, final);
        writeOffset += wrote;
        response->buffer = std::string_view(nullptr, 0);
    }
}

HttpSocket::~HttpSocket()
{
    delete response;
    delete request;
}

} // namespace cW