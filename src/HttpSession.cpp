#include "Server.h"
#include "HttpSession.h"
#include "ClientSocket.h"

namespace cW {

HttpSession::HttpSession(ClientSocket* socket) : Session(socket, Session::HTTP) { dispatch(); }

void HttpSession::dispatch()
{
    auto headerEnd        = socket->receiveBuffer.find("\r\n\r\n");
    requestHeader         = socket->receiveBuffer.substr(0, headerEnd + 2);
    socket->receiveBuffer = socket->receiveBuffer.substr(headerEnd + 4);
    request               = new HttpRequest(requestHeader);
    response              = new HttpResponse();
    // Clock::printElapsed("Dispatching.");
    // reset write state
    socket->server->dispatch(request, response);
    onData();
}
bool HttpSession::shouldEnd()
{
    // writebuffer must be emptied
    return socket->writeBuffer.empty() && doneReceiving && (response->close || doneWriting);
}

bool HttpSession::badRequest()
{
    response->close = true;
    socket->writeBuffer += ("HTTP/1.1 " + HttpStatus::status(HttpStatus::BadRequest) + "\r\n\r\n");
    size_t len         = socket->writeBuffer.size();
    return doneWriting = len == socket->write(nullptr, 0, true);
}

void HttpSession::onData()
{

    try {
        if (request->onDataCallback) {
            doneReceiving = request->onDataCallback(socket->receiveBuffer);
            socket->receiveBuffer.clear();
        }
        else if (request->onBodyCallback) {
            if (request->contentLength == __INF__)
                request->contentLength = request->getHeader<int64_t>("Content-Length");
            if (request->contentLength <= socket->receiveBuffer.size()) {
                request->onBodyCallback(socket->receiveBuffer);
                socket->receiveBuffer.clear();
                doneReceiving = true;
            }
        }
        else
            doneReceiving = true;
    }
    catch (std::runtime_error& error) {
        std::cerr << error.what() << std::endl;
        if (!writing) badRequest();
    }
}

bool HttpSession::onWritable()
{
    if (!wroteHeader) {
        assert(socket->writeBuffer.size() == 0 && "How is size not zero here?");
        socket->writeBuffer += ("HTTP/1.1 " + HttpStatus::status(response->statusCode) + "\r\n");
        for (auto [name, value] : response->headers)
            socket->writeBuffer += (name + ": " + value + "\r\n");
        socket->writeBuffer += "\r\n";
        response->headerLength = socket->writeBuffer.size();
        wroteHeader            = true;
    }
    if (response->onWritableCallback) {
        try {
            response->onWritableCallback(
                writeOffset < response->headerLength ? 0 : writeOffset - response->headerLength);
        }
        catch (std::runtime_error& error) {
            std::cerr << error.what() << std::endl;
            if (!writing) return badRequest();
        }
    }
    if (!response->buffer.empty()) {
        writing             = true;
        size_t bufferLength = response->buffer.length();
        bool   final =
            ((writeOffset < response->headerLength ? 0 : writeOffset - response->headerLength) +
             bufferLength) >= response->contentLength;
        writeOffset += socket->write(response->buffer.data(), bufferLength, final);
        response->buffer   = std::string_view(nullptr, 0);
        return doneWriting = (writeOffset - response->headerLength >= response->contentLength);
    }

    if (!socket->writeBuffer.empty()) writeOffset += socket->write(nullptr, 0, true);
    return doneWriting = socket->writeBuffer.empty();
}
void HttpSession::onAborted()
{
    if (response->onAbortCallback) response->onAbortCallback();
}
HttpSession::~HttpSession()
{
    delete request;
    delete response;
    socket->receiveBuffer.clear();
    socket->writeBuffer.clear();
}

} // namespace cW