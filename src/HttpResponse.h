#ifndef __CW_HTTP_RESPONSE_H_
#define __CW_HTTP_RESPONSE_H_

#include <functional>
#include <map>
#include "Utils.h"
#include "HttpStatusCodes_C++.h"

namespace cW {

class HttpResponse {
    friend class HttpSocket;

    bool close = false;

    typedef std::function<void(void)>   AbortHandler;
    typedef std::function<void(size_t)> WriteHandler;

    WriteHandler onWritableCallback = nullptr;
    AbortHandler onAbortCallback    = nullptr;

    HttpStatus::Code statusCode = HttpStatus::OK;

    std::string_view buffer;
    // send buffer should persist after send call
    std::string sendBuffer;

    bool wroteContentLength = false;

    size_t contentSize;

    std::multimap<std::string, std::string> headers;

  public:
    template <typename T = std::string>
        requires std::is_convertible_v<T, std::string> || requires(T a)
    {
        std::to_string(a);
    }
    HttpResponse* setHeader(const std::string& name, const T& value);
    HttpResponse* setStatus(HttpStatus::Code statusCode);
    void          write(const std::string_view& data, size_t contentSize = __INF__);
    void          send(const std::string& data);
    void          end();
    HttpResponse* onAborted(AbortHandler&& handler);
    HttpResponse* onWritable(WriteHandler&& handler);
};

template <typename T>
    requires std::is_convertible_v<T, std::string> || requires(T a)
{
    std::to_string(a);
}
HttpResponse* HttpResponse::setHeader(const std::string& name, const T& value)
{
    if (ci_match<true>(name, "content-length")) wroteContentLength = true;
    if constexpr (std::is_convertible_v<T, std::string>)
        headers.insert({name, value});
    else
        headers.insert({name, std::to_string(value)});
    return this;
}
}; // namespace cW

#endif