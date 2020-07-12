#ifndef __CW_URL_PARAMS_H_
#define __CW_URL_PARAMS_H_

#include <re2/re2.h>
#include <vector>
#include <map>

namespace cW {
struct Param {
    void*        data;
    unsigned int size;
    bool         isString = false;

    ~Param() { free(data); }
};

class Params {
    friend class HttpRequest;

  protected:
    std::vector<Param>                  params;
    std::map<std::string, unsigned int> indices;
    unsigned int                        size = 0;

    Params(){};

  public:
    // copy constructor
    Params(const Params& other);
    template <class T>
    const T&            get(const std::string& key);
    inline unsigned int count() { return size; }

    ~Params();
};

class UrlParams : public Params {
    // the sizes, later needed for copying
  private:
    std::vector<RE2::Arg*> args;

  public:
    static const RE2 re_urlParam;
    static const RE2 re_strParam;
    static const RE2 re_charParam;
    static const RE2 re_intParam;
    static const RE2 re_fracParam;

    const RE2::Arg* const* getArgs() { return &args[0]; }

    template <class T>
    void add(const std::string& key);
    ~UrlParams();
};

template <class T>
const T& Params::get(const std::string& key)
{
    if (indices.contains(key))
        return *(static_cast<T*>(datas[indices[key]].data));
    else {
        char error[50];
        snprintf(error, 50, "Failure parsing param. URL doesn't contain param %s.", key.c_str());
        throw std::exception(error);
    }
}

template <class T>
void UrlParams::add(const std::string& key)
{
    T*    data;
    Param param;
    // only class type supported
    if (std::is_same<T, std::string>::value) {
        data  = new T();
        param = {.data = data, .isString = true};
    }
    else {
        data  = (T*)malloc(sizeof(T));
        param = {.data = data, .size = sizeof(T)};
    }
    params.push_back(param);
    args.push_back(new RE2::Arg(data));
    indices[key] = size++;
}

}; // namespace cW

#endif