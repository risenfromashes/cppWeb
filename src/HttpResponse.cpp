#include "HttpResponse.h"
#include <iostream>

namespace cW {
HttpResponse* HttpResponse::onWritable(WriteHandler&& handler)
{
    onWritableCallback = std::move(handler);
    return this;
}
HttpResponse* HttpResponse::onAborted(AbortHandler&& handler)
{
    onAbortCallback = std::move(handler);
    return this;
}

void HttpResponse::write(const std::string_view& data, size_t contentSize)
{
    buffer      = data;
    contentSize = contentSize;
    if (contentSize < __INF__ && !wroteContentLength) {
        setHeader("Content-Length", this->contentSize);
        wroteContentLength = true;
    }
}

void HttpResponse::send(const std::string& data)
{
    assert(!onWritableCallback && "Cannot attach write handler and then send data");
    this->contentSize = data.size();
    if (!wroteContentLength) {
        setHeader("Content-Length", this->contentSize);
        wroteContentLength = true;
    }
    sendBuffer = data;
    buffer     = sendBuffer;
}
HttpResponse* HttpResponse::setStatus(HttpStatus::Code statusCode)
{
    this->statusCode = statusCode;
    return this;
}
void HttpResponse::end() { close = true; }

}; // namespace cW