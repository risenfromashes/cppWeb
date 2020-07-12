#ifndef __CW_HTTP_REQUEST_H_
#define __CW_HTTP_REQUEST_H_

#include <re2/re2.h>
#include <map>
#include <string>
#include <cassert>
#include <functional>
#include "UrlParams.h"

namespace cW {

enum HttpMethod { UNSET, GET, POST, PUT, DEL, HEAD };

class HttpRequest {
    friend class HttpSocket;
    friend class WsSocket;

    friend class Router;

    HttpMethod                         method;
    std::string                        absolutePath;
    std::string                        url;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queries;

    Params params;

    typedef std::function<void(std::string_view)> DataHandler;
    DataHandler                                   onDataCallback = nullptr;
    DataHandler                                   onBodyCallback = nullptr;

    // only to be initialized when the headers have been fully received
    HttpRequest(const std::string& receivedData);

  public:
    // callback for more data
    void onData(DataHandler&& onDataCallback);
    // callback for full request body
    void onBody(DataHandler&& onBodyCallback);

    std::string_view getHeader(const std::string& name);

    std::string_view getQuery(const std::string& key);

    template <typename T>
    const T& getParam(const std::string& key);

  private:
    static const RE2 re_statusLine;
    static const RE2 re_absolutePath;
    static const RE2 re_queries;
    static const RE2 re_query;
    static const RE2 re_headerField;
};

}; // namespace cW

#endif