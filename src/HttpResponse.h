#ifndef __CW_HTTP_RESPONSE_H_
#define __CW_HTTP_RESPONSE_H_

#include <functional>
#include <map>
#include "Utils.h"
#include "HttpStatusCodes_C++.h"

namespace cW {

class HttpResponse {
    friend class HttpSocket;

    bool closeSocket = false;

    typedef std::function<void(void)>   AbortHandler;
    typedef std::function<void(size_t)> WriteHandler;

    WriteHandler onWritableCallback = nullptr;
    AbortHandler onAbortCallback    = nullptr;

    HttpStatus::Code statusCode = HttpStatus::OK;

    std::string_view buffer;

    bool wroteContentLength = false;

    size_t contentSize;

    std::multimap<std::string, std::string> headers;

  public:
    HttpResponse* onWritable(WriteHandler&& handler);
    HttpResponse* onAborted(AbortHandler&& handler);
    void          write(std::string_view data, size_t contentSize = __INF__);
    void          send(std::string_view data);
    HttpResponse* setStatus(HttpStatus::Code statusCode);
    HttpResponse* setHeader(const std::string& name, const std::string& value);
    void          end();
};

}; // namespace cW

#endif