#include <iostream>
#include "HttpRequest.h"

namespace cW {

const RE2 HttpRequest::re_statusLine("([A-Z]{3,}) ([^ ]+) HTTP/1.\\d");
const RE2 HttpRequest::re_absolutePath("[^:/](/[^/ ?]*)");

const RE2 HttpRequest::re_queries("\\?(.*)");
const RE2 HttpRequest::re_query("(\\w+)=([^&]+)");

const RE2 HttpRequest::re_headerField("\\s*(\\S+)\\s*:[ ]*([^\r\n]*)\r\n");

HttpRequest::HttpRequest(const std::string& receivedData)
{
    re2::StringPiece input(receivedData);
    std::string      headerName, headerValue;
    std::string      methodStr, url;
    RE2::FindAndConsume(&input, re_statusLine, &methodStr, &url);

    if (methodStr == "GET")
        method = HttpMethod::GET;
    else if (methodStr == "POST")
        method = HttpMethod::POST;
    else if (methodStr == "PUT")
        method = HttpMethod::PUT;
    else if (methodStr == "DELETE")
        method = HttpMethod::DEL;
    else if (methodStr == "HEAD")
        method = HttpMethod::HEAD;

    if (!RE2::PartialMatch(url, re_absolutePath, &absolutePath)) absolutePath = "/";

    re2::StringPiece queries_str;
    std::string      key, val;
    if (RE2::PartialMatch(url, re_queries, &queries_str)) {
        while (RE2::FindAndConsume(&queries_str, re_query, &key, &val))
            queries.insert({key, val});
    }
    while (RE2::FindAndConsume(&input, re_headerField, &headerName, &headerValue))
        headers.insert({headerName, headerValue});
}

// callback for more data
void HttpRequest::onData(DataHandler&& onDataCallback)
{
    this->onBodyCallback = nullptr;
    this->onDataCallback = std::move(onDataCallback);
}
// callback for full request body
void HttpRequest::onBody(DataHandler&& onBodyCallback)
{
    this->onDataCallback = nullptr;
    this->onBodyCallback = std::move(onBodyCallback);
}

std::string_view HttpRequest::getHeader(const std::string& name)
{
    std::map<std::string, std::string>::iterator itr;
    if ((itr = headers.find(name)) != headers.end()) return itr->second;
    return std::string_view(nullptr, 0);
}

std::string_view HttpRequest::getQuery(const std::string& key)
{
    std::map<std::string, std::string>::iterator itr;
    if ((itr = queries.find(key)) != queries.end()) return itr->second;
    return std::string_view(nullptr, 0);
}

template <typename T>
const T& HttpRequest::getParam(const std::string& key)
{
    return params.get<T>(key);
}
}; // namespace cW