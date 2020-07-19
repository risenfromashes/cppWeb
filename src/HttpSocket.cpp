#include "HttpSocket.h"
#include "Server.h"

namespace cW {
HttpSocket::HttpSocket(ClientSocket* other) : ClientSocket(other)
{
    upgraded = true;
    tryDispatch();
}

void HttpSocket::tryDispatch()
{
    auto headerEnd = receiveBuffer.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        gotHeader     = true;
        requestHeader = receiveBuffer.substr(0, headerEnd + 2);
        receiveBuffer = receiveBuffer.substr(headerEnd + 4);
        request       = new HttpRequest(requestHeader);
        response      = new HttpResponse();
        Clock::printElapsed("Dispatching.");
        server->dispatch(request, response);
    }
}

void HttpSocket::badRequest()
{
    writeBuffer += ("HTTP/1.1 " + HttpStatus::status(HttpStatus::BadRequest) + "\r\n\r\n");
    response->close = true;
}

void HttpSocket::loopPreCb()
{
    ClientSocket::loopPreCb();
    if (gotHeader && writeBuffer.empty() &&
        (response->close || writeOffset - responseHeaderLength >= response->contentSize)) {
        disconnect();
    }
}
void HttpSocket::loopPostCb()
{
    if (gotHeader && writeBuffer.empty() &&
        (response->close || writeOffset - responseHeaderLength >= response->contentSize)) {
        disconnect();
    }
    ClientSocket::loopPostCb();
}
void HttpSocket::onData()
{
    if (!gotHeader)
        tryDispatch();
    else {
        try {
            if (request->onDataCallback) {
                request->onDataCallback(std::string_view(receiveBuffer));
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
}

bool HttpSocket::onWritable()
{
    if (gotHeader) {
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
        if (writeBuffer.size() >= MaxWriteSize)
            return bufferOffset >= response->contentSize;
        else {
            if (response->onWritableCallback) {
                try {
                    response->onWritableCallback(bufferOffset);
                }
                catch (std::runtime_error& error) {
                    std::cerr << error.what() << std::endl;
                    if (!writing) badRequest();
                    response->close = true;
                    return true;
                }
            }
            if (!response->buffer.empty()) {
                writing     = true;
                size_t size = response->buffer.size();
                bufferOffset += size;
                bool final = response->contentSize <= (size + bufferOffset);
                writeBuffer.append(response->buffer);
                response->buffer = std::string_view(nullptr, 0);
                return final;
            }
            else
                return false;
        }
    }
    return false;
}

HttpSocket::~HttpSocket()
{
    delete response;
    delete request;
}

} // namespace cW