#include "HttpSession.h"

#include "ClientSocket.h"
#include "Server.h"
#include <sstream>
namespace cW {

HttpSession::HttpSession(ClientSocket* socket, const std::string_view& requestHeader)
    : Session(socket, Session::HTTP)
{
    dispatch(requestHeader);
}

void HttpSession::dispatch(const std::string_view& requestHeader)
{
    request  = new HttpRequest(requestHeader);
    response = new HttpResponse();
    // Clock::printElapsed("Dispatching.");
    // reset write state
    if (hasHandler = socket->server->dispatch(request, response)) {
        if (request->onBodyCallback) request->data.reserve(request->contentLength);
        request->inHandler = false;
    }
    else
        socket->connected = false;
}
bool HttpSession::shouldEnd()
{
    // writebuffer must be emptied
    return !hasHandler ||
           (socket->writeBuffer.empty() && doneReceiving && (response->close || doneWriting));
}

void HttpSession::badRequest()
{
    response->close = true;
    socket->writeBuffer += ("HTTP/1.1 " + HttpStatus::status(HttpStatus::BadRequest) + "\r\n\r\n");
    size_t len  = socket->writeBuffer.size();
    doneWriting = len == socket->write(nullptr, 0, true);
}

void HttpSession::onData(const std::string_view& data)
{
    try {
        if (request->onDataCallback) { doneReceiving = request->onDataCallback(data); }
        else if (request->onBodyCallback) {
            request->data.append(data);
            if (request->contentLength <= request->data.size()) {
                request->onBodyCallback(request->data);
                request->data.clear();
                doneReceiving = true;
            }
        }
        else
            doneReceiving = true;
    }
    catch (std::runtime_error& error) {
        std::cerr << error.what() << std::endl;
        if (!wroteHeader) badRequest();
    }
}

void HttpSession::onWritable()
{
    if (!wroteHeader) {
        // assert(socket->writeBuffer.size() == 0 && "How is size not zero here?");
        socket->write("HTTP/1.1 ");
        socket->write(HttpStatus::status(response->statusCode));
        socket->write("\r\n");
        for (auto [name, value] : response->headers) {
            socket->write(name);
            socket->write(": ");
            socket->write(value);
            socket->write("\r\n");
        }
        socket->write("\r\n");
        wroteHeader = true;
    }
    if (response->onWritableCallback) {
        try {
            response->onWritableCallback(writeOffset);
        }
        catch (std::runtime_error& error) {
            std::cerr << error.what() << std::endl;
            if (!wroteHeader) return badRequest();
        }
    }
    if (!response->buffer.empty()) {
        size_t bufferLength = response->buffer.length();
        bool   final        = (writeOffset + bufferLength) >= response->contentLength;
        writeOffset += socket->write(response->buffer.data(), bufferLength, final);
        response->buffer = std::string_view(nullptr, 0);
        doneWriting      = writeOffset >= response->contentLength;
    }
    else {
        if (!socket->writeBuffer.empty()) socket->write(nullptr, 0, true);
        doneWriting = socket->writeBuffer.empty();
    }
}

void HttpSession::onAwakePre() {}
void HttpSession::onAwakePost() {}

void HttpSession::onAborted()
{
    if (response->onAbortCallback) response->onAbortCallback();
}

HttpSession::~HttpSession()
{
    delete request;
    delete response;
}

} // namespace cW