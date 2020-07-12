#include "UrlParams.h"

namespace cW {

const RE2 UrlParams::re_urlParam("\\{(.{1,3}):(\\w+)\\}");

const RE2 UrlParams::re_strParam("\\{s:\\w+\\}");
const RE2 UrlParams::re_charParam("\\{c:\\w+\\}");
const RE2 UrlParams::re_intParam("\\{(d|u|ld|lu|lld|llu):\\w+\\}");
const RE2 UrlParams::re_fracParam("\\{(f|lf):\\w+\\}");

Params::Params(const Params& other) : indices(other.indices), size(other.size)
{
    for (unsigned int i = 0; i < other.size; i++) {
        void* data;
        if (other.params[i].isString) {
            data = new std::string(*(static_cast<std::string*>(other.params[i].data)));
            params.push_back({.data = data, .isString = true});
        }
        else {
            auto size = other.params[i].size;
            data      = malloc(size);
            memcpy_s(data, size, other.params[i].data, size);
            params.push_back({.data = data, .size = size});
        }
    }
}

Params::~Params()
{
    for (auto&& param : params) {
        if (param.isString)
            delete param.data;
        else
            free(param.data);
    }
    params.clear();
    indices.clear();
}

UrlParams::~UrlParams()
{
    Params::~Params();
    for (auto&& arg : args)
        delete arg;
    args.clear();
}

}; // namespace cW