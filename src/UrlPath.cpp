#include "UrlPath.h"

namespace cW {
UrlParam::UrlParam(const void* val, size_t size, Type type) : type(type), size(size)
{
    data = malloc(size + (int)(type == Type::STRING));
    std::memcpy(data, val, size);
    if (type == Type::STRING) ((char*)data)[size] = '\0';
}

UrlParam::~UrlParam() { free(data); }

UrlPath::UrlPath(const std::string_view& absPath)
{
    assert(absPath[0] == '/' && "Absolute path must start with /");
    size_t len = absPath.size();
    for (size_t i = 1; i < len;) {
        size_t next    = std::min(absPath.find('/', i), len);
        auto   segment = absPath.substr(i, next - i);
        // param
        if (segment[0] == '{') {
            size_t segmentLen = segment.size();
            assert((segment[1] == 's' || segment[1] == 'd' || segment[1] == 'f') &&
                   (segment[2] == ':') && (segment[segmentLen - 1] == '}') &&
                   "Invalid type or format");
            UrlLevel level{.val = segment.substr(3, segmentLen - 4)};
            switch (segment[1]) {
                case 's': level.type = UrlLevel::Type::PARAM_STRING; break;
                case 'd': level.type = UrlLevel::Type::PARAM_INT; break;
                case 'f': level.type = UrlLevel::Type::PARAM_FLOAT; break;
            }
            levels.push_back(level);
        }
        // wildcard
        else if (segment == "*")
            levels.push_back(UrlLevel{.type = UrlLevel::WILDCARD});
        // absolute
        else
            levels.push_back(UrlLevel{.type = UrlLevel::ABOSULUTE, .val = segment});
        i = next + 1;
    }
}

bool UrlPath::operator==(const std::string_view& absPath) const
{
    assert(absPath[0] == '/' && "Absolute path must start with /");
    size_t in_levels = count_char('/', absPath.data(), absPath.size());
    if (absPath.back() == '/') in_levels--;
    const char*  path = absPath.data();
    const size_t len  = absPath.size();
    bool         last_match_was_wildcard;
    for (size_t i = 1, level = 0; i < len && level < levels.size(); level++) {
        last_match_was_wildcard = false;
        size_t next             = std::min(absPath.find('/', i), len);
        switch (levels[level].type) {
            case UrlLevel::Type::WILDCARD: last_match_was_wildcard = true; break;
            case UrlLevel::Type::PARAM_STRING: break;
            case UrlLevel::Type::ABOSULUTE: {
                auto val = levels[level].val;
                if (val.size() != (next - i)) return false;
                for (size_t j = 0; j < val.size(); j++)
                    if (path[i + j] != val[j]) return false;
                break;
            }
                char* end;
            case UrlLevel::Type::PARAM_INT: {
                for (size_t j = i; j < next; j++)
                    if (!isdigit(path[i])) return false;
                break;
            }
            case UrlLevel::Type::PARAM_FLOAT: {
                for (size_t j = i; j < next; j++)
                    if (!isdigit(path[i]) && path[i] != '.') return false;
                break;
            }
        }
        i = next + 1;
    }
    // printf("In levels: %ld, Levels: %ld\n", in_levels, levels.size());
    return in_levels == levels.size() || last_match_was_wildcard;
}

std::map<std::string_view, UrlParam*> UrlPath::parseParams(const std::string_view& absPath) const
{
    // assert(*this == absPath && "Must match url to parse params");
    const char*                           path = absPath.data();
    const size_t                          len  = absPath.size();
    std::map<std::string_view, UrlParam*> params;
    for (size_t i = 1, level = 0; i < len && level < levels.size(); level++) {
        size_t next = std::min(absPath.find('/', i), len);
        switch (levels[level].type) {
            case UrlLevel::Type::PARAM_INT: {
                int64_t value = strtoll(path + i, nullptr, 10);
                params.insert({levels[level].val,
                               new UrlParam(&value, sizeof(int64_t), UrlParam::Type::INT)});
                break;
            }
            case UrlLevel::Type::PARAM_FLOAT: {
                long double value = strtold(path + i, nullptr);
                params.insert({levels[level].val,
                               new UrlParam(&value, sizeof(long double), UrlParam::Type::FLOAT)});
                break;
            }
            case UrlLevel::Type::PARAM_STRING:
                params.insert(
                    {levels[level].val, new UrlParam(path + i, next - i, UrlParam::Type::STRING)});
                break;
        }
        i = next + 1;
    }
    return params;
}

UrlPath::~UrlPath() {}

}; // namespace cW