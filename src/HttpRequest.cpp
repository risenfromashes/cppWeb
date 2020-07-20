#include <iostream>
#include "HttpRequest.h"

namespace cW {

bool HttpRequest::HeaderComp::operator()(const std::string_view& a, const std::string_view& b) const
{
    size_t i     = 0;
    size_t a_len = a.size(), b_len = b.size();
    while (true) {
        if (tolower(a[i]) < tolower(b[i])) return true;
        if (tolower(a[i]) > tolower(b[i])) return false;
        i++;
        if (i == a_len || a[i] == ' ' || a[i] == ':') {
            if (i == b_len || b[i] == ' ' || b[i] == ':')
                return false; // equal
            else
                return true; // a shorter
        }
        else if (i == b_len || b[i] == ' ' || b[i] == ':')
            return false; // b shorter
    }
}

bool HttpRequest::QueryComp::operator()(const std::string_view& a, const std::string_view& b) const
{
    size_t i     = 0;
    size_t a_len = a.size(), b_len = b.size();
    while (true) {
        if (a[i] < b[i]) return true;
        if (a[i] > b[i]) return false;
        i++;
        if (i == a_len || a[i] == '=') {
            if (i == b_len || b[i] == '=')
                return false; // equal
            else
                return true; // a shorter
        }
        else if (i == b_len || b[i] == '=')
            return false; // b shorter
    }
}

HttpRequest::HttpRequest(const std::string_view& requestHeader)
{
    std::string_view _method;
    auto             first_space  = requestHeader.find(' ');
    auto             second_space = requestHeader.find(' ', first_space + 1);
    _method                       = requestHeader.substr(0, first_space);

    if (_method == "GET")
        method = HttpMethod::GET;
    else if (_method == "POST")
        method = HttpMethod::POST;
    else if (_method == "PUT")
        method = HttpMethod::PUT;
    else if (_method == "DELETE")
        method = HttpMethod::DEL;
    else if (_method == "HEAD")
        method = HttpMethod::HEAD;

    url           = requestHeader.substr(first_space + 1, second_space - first_space - 1);
    size_t offset = 0;
    if (auto doubleSlash = url.find("://"); doubleSlash != std::string::npos)
        offset = doubleSlash + 3;
    size_t queryStart = url.find('?');
    if (auto slash = url.find('/', offset); slash != std::string::npos)
        absolutePath = url.substr(slash, queryStart - slash);
    else
        absolutePath = "/";

    size_t statusLineEnd = requestHeader.find("\r\n") + 2;
    querySection         = (queryStart == std::string_view::npos) ? "" : url.substr(queryStart + 1);
    headerSection        = requestHeader.substr(statusLineEnd);
}

// callback for more data
HttpRequest* HttpRequest::onData(DataHandler&& onDataCallback)
{
    this->onBodyCallback = nullptr;
    this->onDataCallback = std::move(onDataCallback);
    return this;
}
// callback for full request body
HttpRequest* HttpRequest::onBody(DataHandler&& onBodyCallback)
{
    this->onDataCallback = nullptr;
    this->onBodyCallback = std::move(onBodyCallback);
    return this;
}

HttpRequest::~HttpRequest()
{
    for (auto [key, param] : params)
        delete param;
    params.clear();
}

}; // namespace cW