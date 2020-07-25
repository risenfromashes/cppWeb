#ifndef __CW_HTTP_REQUEST_H_
#define __CW_HTTP_REQUEST_H_

#include <map>
#include <string>
#include <cassert>
#include <functional>
#include <set>
#include <stdexcept>
#include "UrlPath.h"

namespace cW {

enum HttpMethod { UNSET, GET, POST, PUT, DEL, HEAD };

class HttpRequest {
    friend class HttpSession;
    friend class WebSocketSession;
    friend class Router;

    struct HeaderComp {
        bool operator()(const std::string_view& a, const std::string_view& b) const;
    };
    struct QueryComp {
        bool operator()(const std::string_view& a, const std::string_view& b) const;
    };

    HttpMethod       method;
    std::string_view url;
    std::string_view absolutePath;

    std::string data;
    bool        inHandler = true;

    const UrlPath* urlPath;

    std::string_view                       headerSection;
    std::string_view                       querySection;
    std::set<std::string_view, HeaderComp> headers;
    std::set<std::string_view, QueryComp>  queries;
    std::map<std::string_view, UrlParam*>  params;

    bool paramsParsed    = false;
    bool headersSplitted = false;
    bool queriesSplitted = false;

    size_t contentLength = __INF__;

    std::function<bool(std::string_view)> onDataCallback = nullptr;
    std::function<void(std::string_view)> onBodyCallback = nullptr;
    // only to be initialized when the headers have been fully received
    HttpRequest(const std::string_view& requestHeader);
    void parse(const std::string_view& requestHeader);

    inline void getContentLength();

  public:
    ~HttpRequest();
    // callback for more data
    HttpRequest* onData(std::function<bool(std::string_view)>&& onDataCallback);
    // callback for full request body
    HttpRequest* onBody(std::function<void(std::string_view)>&& onBodyCallback);

    template <typename T = std::string_view>
        requires std::is_arithmetic_v<T> ||
        std::is_convertible_v<std::string_view, T> const T getHeader(const std::string_view& key);

    template <typename T = std::string>
        requires std::is_arithmetic_v<T> ||
        std::is_convertible_v<std::string, T> const T getQuery(const std::string_view& key);

    template <typename T = std::string>
        requires std::is_arithmetic_v<T> ||
        std::is_convertible_v<std::string, T> const T getParam(const std::string_view& key);
};

void HttpRequest::getContentLength()
{
    static const char* headerName    = "content-length";
    static const int   headerNameLen = strlen(headerName);
    size_t             start;
    if ((start = ci_find<true>(headerSection, headerName)) < __INF__) {
        start += headerNameLen;
        while (headerSection[start] == ' ' || headerSection[start] == ':')
            start++;
        contentLength = strtoull(headerSection.data() + start, nullptr, 10);
    }
    else
        contentLength = 0;
}

template <typename T>
    requires std::is_arithmetic_v<T> ||
    std::is_convertible_v<std::string, T> const T HttpRequest::getQuery(const std::string_view& key)
{
    assert(inHandler &&
           "Cannot access request information outside of route handler or inside data handler");
    static QueryComp queryComp;
    if (!queriesSplitted) {
        size_t len = querySection.size();
        for (size_t i = 0; i < len;) {
            size_t next = std::min(querySection.find('&', i), len);
            queries.insert(querySection.substr(i, next - i));
            i = next + 1;
        }
        queriesSplitted = true;
    }
    auto&& itr  = std::lower_bound(queries.begin(), queries.end(), key, queryComp);
    auto&& line = *itr;
    if (!queryComp(line, key)) {
        // equal
        size_t lineLen = line.size();
        size_t first = line.find('=') + 1, last = lineLen - 1;
        char*  decoded = url_decode(line.data() + first, last - first + 1);
        T      ret;
        if constexpr (std::is_arithmetic_v<T>) {
            char* end;
            if constexpr (std::is_integral_v<T>)
                ret = (T)strtoll(decoded, &end, 10);
            else
                ret = (T)strtold(decoded, &end);
            if (*end) std::runtime_error("Query value is not arithmatic.");
        }
        else
            ret = std::string(decoded);
        free(decoded);
        return ret;
    }
    else
        throw std::runtime_error("Query not found");
}

template <typename T>
    requires std::is_arithmetic_v<T> ||
    std::is_convertible_v<std::string, T> const T HttpRequest::getParam(const std::string_view& key)
{
    assert(inHandler &&
           "Cannot access request information outside of route handler or inside data handler");
    if (!paramsParsed) {
        params       = urlPath->parseParams(url);
        paramsParsed = true;
    }
    if (auto itr = params.find(key); itr != params.end())
        return itr->second->get<T>();
    else
        throw std::runtime_error("Asking for non-existent param");
}

template <typename T>
    requires std::is_arithmetic_v<T> || std::is_convertible_v<std::string_view, T> const T
                                        HttpRequest::getHeader(const std::string_view& key)
{
    assert(inHandler &&
           "Cannot access request information outside of route handler or inside data handler");
    static HeaderComp headerComp;
    if (!headersSplitted) {
        size_t len = headerSection.size();
        for (size_t i = 0; i < len;) {
            size_t next = std::min(headerSection.find("\r\n", i), len);
            headers.insert(headerSection.substr(i, next - i));
            i = next + 2;
        }
        headersSplitted = true;
    }
    auto&& itr  = std::lower_bound(headers.begin(), headers.end(), key, headerComp);
    auto&& line = *itr;
    if (!headerComp(line, key)) {
        // equal
        size_t lineLen = line.size();
        size_t first = line.find(':') + 1, last = lineLen - 1;
        while (line[first] == ' ')
            first++;
        while (line[last] == ' ')
            last--;
        if constexpr (std::is_arithmetic_v<T>) {
            char* end;
            if constexpr (std::is_integral_v<T>)
                return (T)strtoll(line.data() + first, &end, 10);
            else
                return (T)strtold(line.data() + first, &end);
            if (*end != '\r') std::runtime_error("Header value is not arithmatic.");
        }
        else
            return (T)std::string_view(line.data() + first, last - first + 1);
    }
    else
        throw std::runtime_error("Header not found");
}

} // namespace cW

#endif