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

void HttpResponse::write(std::string_view data, size_t contentSize)
{
    writeBuffer       = data;
    this->contentSize = contentSize;
}

void HttpResponse::send(std::string_view data)
{
    onWritableCallback = [this, data](size_t offset) {
        if (offset < data.size())
            writeBuffer = std::string_view(data.data(), data.size() - offset);
        else
            closeSocket = true;
    };
}

HttpResponse* HttpResponse::setStatus(HttpStatus::Code statusCode)
{
    this->statusCode = statusCode;
    return this;
}
HttpResponse* HttpResponse::setHeader(const std::string& name, const std::string& value)
{
    headers.insert({name, value});
    return this;
}
void HttpResponse::end() { closeSocket = true; }

}; // namespace cW