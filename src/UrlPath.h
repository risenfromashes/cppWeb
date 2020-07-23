#ifndef __CW_URL_PATH_H_
#define __CW_URL_PATh_H_

#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include "Utils.h"
namespace cW {

struct UrlParam {
    friend struct UrlPath;

  private:
    enum Type { INT, FLOAT, STRING };
    void*  data;
    Type   type;
    size_t size;

    UrlParam(const void* val, size_t size, Type type);

  public:
    template <typename T>
    requires std::is_arithmetic_v<T> const T get() const
    {
        if (type == Type::STRING) throw std::runtime_error("Requesting number from string param");
        switch (type) {
            case INT: return (T)(*(static_cast<int64_t*>(data)));
            case FLOAT: return (T)(*(static_cast<long double*>(data)));
        }
        return 0;
    }

    template <typename T = std::string>
    requires std::is_convertible_v<std::string, T> const T get() const
    {
        switch (type) {
            case STRING: return std::string((const char*)data);
            case INT: return std::to_string(*(static_cast<int64_t*>(data)));
            case FLOAT: return std::to_string(*(static_cast<long double*>(data)));
        }
        return "";
    }
    ~UrlParam();
};

class UrlPath {

  public:
    struct UrlLevel {
        enum Type { ABOSULUTE, WILDCARD, PARAM_INT, PARAM_FLOAT, PARAM_STRING };
        Type type;
        // param name or absolute string
        std::string_view val;
    };
    std::vector<UrlLevel> levels;
    UrlPath(const std::string_view& absPath);
    bool                                  operator==(const std::string_view& absPath) const;
    std::map<std::string_view, UrlParam*> parseParams(const std::string_view& absPath) const;
    ~UrlPath();
};

}; // namespace cW

#endif