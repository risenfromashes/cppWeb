#ifndef __CW_JSON_H_
#define __CW_JSON_H_

#include "Utils.h"

namespace cW {
// clang-format off


template<FixedString prefix>
class RuntimeError : public std::runtime_error{
    template <typename T>
        requires requires(T a, std::ostream &os){ os << a;}
    static inline void __write(std::ostream &os, T arg){ os << " " << arg; }
    template <typename T, typename... R>
        requires requires(T a, std::ostream &os){ os << a;}
    static inline void __write(std::ostream &os, T arg, R... args)
    {
        os << " " << arg;
        __write(os, args...);
    }
    // clang-format on
    static std::string errorString;

  public:
    RuntimeError() : std::runtime_error(prefix) {}
    template <typename P, typename... R>
    static inline RuntimeError format(P* toDelete, const std::string& message, R... args)
    {
        if (toDelete) delete toDelete;
        std::ostringstream oss;
        oss << prefix << message;
        if (sizeof...(args) > 0) __write(oss, args...);
        errorString = oss.str();
        return RuntimeError();
    }
    template <typename... R>
    static inline void format(const std::string& message, R... args)
    {
        return format(nullptr, message, args...);
    }
    const char* what() const noexcept override { return errorString.c_str(); }
};

std::string RuntimeError::errorString;

struct JsonNode {
    enum Type { Value, Array, Object };
    const Type type;
    virtual ~JsonNode() {}
    template <typename T>
        requires __JsonReturnVal<T> || __JsonReturnArray<T> T get(const std::string_view& path);

  protected:
    JsonNode(Type type) : type(type) {}
};
struct JsonValue : JsonNode {
    enum ValueType { Bool, Number, Null, String };
    const ValueType valueType;
    ~JsonValue() override {}

  protected:
    JsonValue(ValueType valType) : JsonNode{JsonNode::Value}, valueType(valType) {}
};

struct JsonString : JsonValue {
    std::string_view value;
    JsonString(const std::string_view& value) : JsonValue(JsonValue::String), value() {}
};
struct JsonNull : JsonValue {
    JsonNull() : JsonValue(JsonValue::Null) {}
};
struct JsonBool : JsonValue {
    bool value;
    JsonBool(bool value) : JsonValue(JsonValue::Bool), value(value) {}
};
struct JsonNumber : JsonValue {
    const char* value;
    JsonNumber(const char* start_ptr) : JsonValue(JsonValue::Number), value(start_ptr) {}
    template <typename T>
    requires std::is_arithmetic_v<T> T get()
    {
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>)
                return (T)strtoull(value, nullptr, 10);
            else
                return (T)strtoll(value, nullptr, 10);
        }
        else if constexpr (std::is_arithmetic_v<T>)
            return (T)strtold(value, nullptr);
    }
};

struct JsonArray : JsonNode {
    std::vector<JsonNode*> elements;
    JsonArray() : JsonNode{JsonNode::Array} {}
    ~JsonArray() override
    {
        for (auto node : elements)
            delete node;
        elements.clear();
    }
};

struct JsonObject : JsonNode {
    std::map<std::string_view, JsonNode*> entries;
    JsonObject() : JsonNode{JsonNode::Object} {}
    ~JsonObject() override
    {
        for (auto [key, node] : entries)
            delete node;
        entries.clear();
    }
};

// clang-format off
#ifdef __cpp_concepts
template <typename T, class _Class_T>
concept __JsonEntry = requires(T, std::ostream& os, _Class_T* ptr, JsonNode* node){
    { T::key }->std::convertible_to<const char*>;
    T::write(ptr, os);
    T::set(ptr, node);
    //{T::toJson(ptr) }->std::convertible_to<std::string>;
};

template <typename T>
concept __JsonVal = std::is_convertible_v<T, const char*> ||
                    (!std::is_array_v<clean_t<T>> &&
                    (  requires(clean_t<T> a, const char* v, size_t len) { (decltype(a))(v,len); } || // string
                       requires(clean_t<T> a, int v) { (decltype(a))(v); } || // number or bool
                       requires(clean_t<T> a, JsonObject* v) { (decltype(a))(v); } // object
                    ) &&
                     ( requires(clean_t<T> val, std::ostream& os) { os << val; }                              ||
                       requires(clean_t<T> val, std::ostream& os) { val.writeJson(os); }                      ||
                       requires(clean_t<T> val) { { val.toString() } ->std::convertible_to<std::string>; }    ||
                       requires(clean_t<T> val) {{val.toJson() }->std::convertible_to<std::string>;
                    }));

template <typename _Container_T>
concept __JsonContainer = requires(_Container_T a)
{
    a.begin();
    ++(a.begin());
    a.end();
    a.size();
    requires requires(clean_t<decltype(*a.begin())> t)
    {
        requires requires { a.push_back(t); }
        || requires { a.insert(t); };
    };
};

// upto 3D array
template <typename _Container_T>
concept __JsonArray = ( std::is_bounded_array_v<clean_t<_Container_T>> &&
                        requires(clean_t<_Container_T> a) {
                          requires __JsonVal<clean_t<decltype(a[0])>> ||
                            std::is_bounded_array_v<clean_t<decltype(a[0])>> &&
                            requires __JsonVal<clean_t<decltype(a[0][0])>> ||
                              std::is_bounded_array_v<clean_t<decltype(a[0][0])>> &&
                              requires __JsonVal<clean_t<decltype(a[0][0][0])>>;
                        };) ||
                      requires(clean_t<_Container_T> a){
                        requires __JsonContainer<decltype(a)>;
                        requires __JsonVal<clean_t<decltype(*a.begin())>> || 
                            requires(clean_t<decltype(*a.begin())> _a)
                            { requires __JsonContainer<decltype(_a)>;
                              requires __JsonVal<clean_t<decltype(*_a.begin())>> ||
                              requires(clean_t<decltype(*_a.begin())> __a)
                                { requires __JsonContainer<decltype(__a)>;
                                  requires __JsonVal<clean_t<decltype(*__a.begin())>>;
                                };
                           };
                        };

template <typename T>
concept __Jsonable =
    is_specialization_v<typename T::Json::Context, JsonContext> && !is_ref_v<T> &&
    std::is_same_v<clean_t<T>, _Class_T> &&
    (std::is_pointer_v<T> || std::is_copy_constructible<clean_t<T>>)&&requires(const JsonObject* a)
{
    T(a);
};

template <FixedString _key, auto _member_ptr>
    requires std::is_member_pointer_v<decltype(_member_ptr)> &&
     (__JsonVal<member_pointer_class_t<_member_ptr>> || __JsonArray<member_pointer_class_t<_member_ptr>>) 
struct JsonEntry {
    using error = RuntimeError<"Invalid conversion from json to object.">;
    using _Clean_T = clean_t<_Member_t>;
    static constexpr const char* key        = _key;
    static constexpr const bool  is_pointer = std::is_pointer_v<_Member_T>;
    static constexpr const bool  is_ref     = !is_pointer && is_ref_v<_Member_T>;
    static std::map<_Class_T*, std::vector<_Clean_T*>> refPtrs;
    static inline const _Member_T& get(const _Class_T* _this)
    {
        return *((const _Member_T*)(size_t(_this) + _offset));
    }
    static inline _Clean_T* getPtr(_Class_T* _this)
    {
        if(is_)
        return (_Clean_T*)(size_t(_this) + _offset);
    }

    template <typename T>
    static auto getRef(const T& val)
        -> std::add_lvalue_reference_t<std::add_const_t<std::remove_pointer_t<T>>>
    {
        if constexpr (is_pointer)
            return *val;
        else
            return val;
    }

    template <typename T>
    static void writeVal(const T& val, std::ostream& os)
    {
        if constexpr (std::is_same_v<clean_t<T>, bool>) {
            if (val)
                os << "true";
            else
                os << "false";
        }
        else if constexpr (std::is_arithmetic_v<clean_t<T>>)
            os << val;
        else if constexpr (requires(clean_t<T> val, std::ostream & os) { os << val; })
            os << '\"' << val << '\"';
        else if constexpr (requires(clean_t<T> val, std::ostream & os) { val.writeJson(os); })
            val.writeJson(os);
        else if constexpr (requires(clean_t<T> val) {{val.toJson()}->std::convertible_to<std::string>;})
            os << val.toJson();
        else if constexpr (requires(clean_t<T> val) {{ val.toString()}->std::convertible_to<std::string>;})
            os << '\"' << val.toString() << '\"';
        else
            os << "\"\"";
    }

    template <typename _Array_T, unsigned int rank, unsigned int level>
    static void writeArray(const _Array_T& arr, std::ostream& os)
    {
        os << "[";
        using Tp             = decltype(arr[0]);
        using T              = clean_t<Tp>;
        constexpr bool isPtr = std::is_pointer_v<Tp>;
        for (int i = 0; i < std::extent_v<_Array_T>; i++) {
            const T& valp = arr[i];
            if constexpr (isPtr) {
                if (arr[i] == nullptr) {
                    os << "null";
                    continue;
                }
                if constexpr (std::is_convertible_v<Tp, const char*>) {
                    os << '\"' << valp << '\"';
                    continue;
                }
            }
            const T& val = getRef(valp);
            if constexpr (level < rank - 1)
                writeArray<T, rank, level + 1>(val, os);
            else
                writeVal(val, os);
            if (i < std::extent_v<_Array_T> - 1) os << ",";
        }
        os << "]";
    }

    template <typename _List_T>
    static void writeList(const _List_T& arr, std::ostream& os)
    {
        os << "[";
        using Tp             = std::remove_cvref_t<decltype(*arr.begin())>;
        using T              = std::remove_pointer_t<Tp>;
        constexpr bool isPtr = std::is_pointer_v<Tp>;
        const size_t   size  = arr.size();
        size_t         i     = 0;
        for (auto itr = arr.begin(); itr != arr.end(); ++itr) {
            const Tp& valp = *itr;
            if constexpr (isPtr) {
                if (valp == nullptr) {
                    os << "null";
                    continue;
                }
                if constexpr (std::is_convertible_v<Tp, const char*>) {
                    os << '\"' << valp << '\"';
                    continue;
                }
            }
            const T& val = getRef(*itr);
            if constexpr (__JsonVal<T>)
                writeVal(val, os);
            else
                writeList<T>(val, os);
            if (++i < size) os << ",";
        }
        os << "]";
    }

    static void write(const _Class_T* _this, std::ostream& os)
    {
        using Tp = std::remove_cvref_t<_Member_T>;
        using T  = std::remove_pointer_t<Tp>;
        os << '\"' << key << "\": ";
        const Tp& valp = get(_this);
        if constexpr (std::is_pointer_v<Tp>) {
            if (!valp) {
                os << "null";
                return;
            }
        }
        if constexpr (std::is_convertible_v<Tp, const char*>) {
            os << '\"' << valp << '\"';
            return;
        }
        const T& val = getRef(valp);
        if constexpr (__JsonVal<T>)
            writeVal(val, os);
        else if constexpr (std::is_bounded_array_v<T>)
            writeArray<T, std::rank_v<T>, 0>(val, os);
        else {
            writeList<T>(val, os);
        }
    }

    static void set(_Class_T* _this, JsonNode* node)
    {
        using Tp       = std::remove_cvref_t<_Member_T>
        using T        = std::remove_pointer_t<Tp>;
        Tp* ptr = getPtr(_this);
        switch (node->type) {
            case JsonNode::Value: {
                switch (((JsonValue*)node)->valueType) {
                    case JsonValue::Bool: {
                        auto jbool = static_cast<JsonBool*>(node);
                    }
                    case JsonValue::Number: {
                        auto jbool = static_cast<JsonBool*>(node);
                    }
                    case JsonValue::Null: {
                        if constexpr (is_pointer) *ptr = nullptr;
                        else throw error::format("Member", key, "is not a pointer. But found null in json");
                        break;
                    }
                    case JsonValue::String: {
                        auto jstr = static_cast<JsonBool*>(node);
                        if constexpr(requires(const char* v, size_t len){ T(v, len);}){
                            if constexpr(is_pointer)
                                *ptr = new T(jstr->value.data(), jstr->value.size());
                            else                             
                                new(ptr) T(jstr->value.data(), jstr->value.size());
                        }
                        else throw error::forma("Member", key, "is not constructible from string value.");
                        break;
                    }
                }
            }
            case JsonNode::Object:
            case JsonNode::Array:
        }
        const Tp& valp = get(_this);
        if constexpr (std::is_pointer_v<Tp>) {
            if (!valp) {
                os << "null";
                return;
            }
        }
        if constexpr (std::is_convertible_v<Tp, const char*>) {
            os << '\"' << valp << '\"';
            return;
        }
        const T& val = getRef(valp);
        if constexpr (__JsonVal<T>)
            writeVal(val, os);
        else if constexpr (std::is_bounded_array_v<T>)
            writeArray<T, std::rank_v<T>, 0>(val, os);
        else {
            writeList<T>(val, os);
        }
    }
};

// clang-format on

template <class _Class_T, __JsonEntry<_Class_T>... _Entries>
struct JsonContext {
    static constexpr size_t entry_count = sizeof...(_Entries);
    template <size_t index, __JsonEntry<_Class_T> _entry, __JsonEntry<_Class_T>... _entries>
    static void _write(const _Class_T* ptr, std::ostream& os)
    {
        _entry::write(ptr, os);
        if constexpr (index > 0) {
            os << ",";
            _write<index - 1, _entries...>(ptr, os);
        }
    }
    template <size_t index, __JsonEntry<_Class_T> _entry, __JsonEntry<_Class_T>... _entries>
    static void _set(_Class_T* ptr, JsonObject* jsonObject)
    {
        auto itr = jsonObject->entries.find(_entry::key);
        if (itr != jsonObject->entries.end()) _entry::set(ptr, itr->second);
        if constexpr (index > 0) _set<index - 1, _entries...>(ptr, jsonObject);
    }
    template <size_t index, __JsonEntry<_Class_T> _entry, __JsonEntry<_Class_T>... _entries>
    static void _dtor(_Class_T* ptr)
    {
        _entry::dtor(ptr);
        if constexpr (index > 0) _dtor<index - 1, _entries...>(ptr);
    }
    static void write(const _Class_T* ptr, std::ostream& os)
    {
        os << "{";
        _write<entry_count - 1, _Entries...>(ptr, os);
        os << "}";
    }
    static T set(_Class_T* _this, JsonObject* jsonObject)
    {
        _set<entry_count - 1, _Entries...>(_this, jsonObject);
    }
    static T dtor(_Class_T* _this) { _dtor<entry_count - 1, _Entries...>(_this); }
};
// clang-format on

#define JSONENTRY(type, mem) \
    , cW::JsonEntry<type, decltype(type::mem), #mem, cW::offset_of(&type::mem)>
#define JSONABLE                                                      \
    friend struct cW::JsonContext;                                    \
    struct Json;                                                      \
    inline void        writeJson(std::ostream& os = std::cout) const; \
    inline std::string toJson() const                                 \
    {                                                                 \
        std::ostringstream oss;                                       \
        writeJson(oss);                                               \
        return oss.str();                                             \
    }
#define SET_JSONABLE(type, ...)                                                                   \
    struct type::Json {                                                                           \
        using Context = cW::JsonContext<type CW_MACRO_FOR_EACH_EX(JSONENTRY, type, __VA_ARGS__)>; \
    };                                                                                            \
    void type::writeJson(std::ostream& os) const { type::Json::Context::write(this, os); }

#else

struct JsonContext;
#define JSONABLE                                                      \
  public:                                                             \
    inline void        writeJson(std::ostream& os = std::cout) const; \
    inline std::string toJson() const;
#define SET_JSONABLE(type, ...)

#endif

struct JsonNode;

// clang-format off

template <typename T>
concept __JsonReturnVal = requires __Jsonable<T> || //parsable
                          std::is_same_v<cW::clean_t<T>, JsonNode> ||
                          std::is_base_of_v<JsonNode, cW::clean_t<T>> || // NodeType
                          std::is_arithmetic_v<T> || std::is_convertible_v<std::string_view, T> ||
                          requires(std::string_view a){ T(a);}; // ValueType
template <typename T>
concept __JsonReturnArray_ = requires(T a, size_t n){ a.size(); a.resize(n); };

template <typename T>
concept __JsonReturnArray = std::is_convertible_v<std::vector<JsonNode*>, T> ||
         requires(T a){ requires
                           __JsonReturnArray_<decltype(a)>       && __JsonReturnVal<cW::clean_t<decltype(a[0])>> 
                        || __JsonReturnArray_<decltype(a[0])>    && __JsonReturnVal<cW::clean_t<decltype(a[0][0])>> 
                        || __JsonReturnArray_<decltype(a[0][0])> && __JsonReturnVal<cW::clean_t<decltype(a[0][0][0])>>; };


template <typename T>
    requires __JsonReturnVal<T> ||
    __JsonReturnArray<T> T JsonNode::get(const std::string_view& path)
{
    if (path.empty()) {
        if constexpr (std::is_same_v<cW::clean_t<T>, JsonNode> ||
                      std::is_base_of_v<JsonNode, cW::clean_t<T>>) {
            if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
                return (T)this;
            else
                return *((T*)this);
        }
        if (type == Value) {
            JsonValue* node = static_cast<JsonValue*>(this);
            if constexpr (std::is_arithmetic_v<T>) {
                if (node->valueType != JsonValue::Number)
                    throw std::runtime_error("Json value is not arithmetic");
                return ((JsonNumber*)node)->get<T>();
            }
            else if constexpr (std::is_same_v<T, bool>) {
                if (node->valueType != JsonValue::Bool)
                    throw std::runtime_error("Json value is not boolean");
                return ((JsonBool*)node)->value;
            }
            else if constexpr (std::is_convertible_v<std::string_view, T>) {
                if (node->valueType != JsonValue::String)
                    throw std::runtime_error("Json value is not string");
                return (T)((JsonString*)node)->value;
            }
            else if constexpr (requires(std::string_view a) { T(a); }) {
                if (node->valueType != JsonValue::String)
                    throw std::runtime_error("Json value is not string");
                return T(((JsonString*)node)->value);
            }
        }
        else if (type == Array) {
            JsonArray* node = static_cast<JsonArray*>(this);
            if constexpr (std::is_convertible_v<std::vector<JsonNode*>, T>)
                return node->elements;
            else if constexpr (__JsonReturnArray<T>) {
                using _T = std::remove_cvref_t<T>;
                _T     arr;
                size_t len = node->elements.size();
                arr.resize(len);
                for (size_t i = 0; i < len; i++) {
                    arr[i] = node->elements[i]->get<std::remove_cvref_t<decltype(arr[0])>>("");
                }
                return arr;
            }
        }
        throw std::runtime_error("Asking for invalid value");
    }
    if (path[0] != '/') throw std::runtime_error("Invalid path");
    size_t           slash    = path.find('/', 1);
    std::string_view key      = path.substr(1, slash - 1);
    std::string_view nextPath = slash < std::string_view::npos ? path.substr(slash) : "";
    if (type == Object) {
        JsonObject* node = static_cast<JsonObject*>(this);
        auto        itr  = node->entries.find(key);
        if (itr == node->entries.end()) { throw std::runtime_error("Entry not found"); }
        return itr->second->get<T>(nextPath);
    }
    if (type == Array) {
        JsonArray*  node   = static_cast<JsonArray*>(this);
        size_t      keyLen = key.size();
        char*       end;
        const char* start = key.data();
        size_t      index = strtoll(start, &end, 10);
        if (end - start < keyLen) throw std::runtime_error("Invalid index on array node");
        if (index >= node->elements.size()) throw std::runtime_error("Array index Out of Bound");
        return node->elements[index]->get<T>(nextPath);
    }
    throw std::runtime_error("Invalid path");
}


struct JsonParser {
    using error = RuntimeError<"Invalid Json.">;
    // clang-format off
    static inline size_t skipSpaces(JsonNode* node, const std::string_view& data, size_t& i)
    {
        while (true) {
            switch (data[i]) {
                case ' ':
                case '\t':
                case '\n': break;
                default: return i; ;
            }
            if (i++ >= data.size()) throw error::format(node, "Unexpected end.");
        }
        return i;
    }
    static inline size_t skipToStringEnd(JsonNode* node, const std::string_view& data, size_t& i)
    {
        bool slash = false;
        while (true) {
            if (data[i] == '\"' && !slash) return i;
            slash = data[i] == '\\';
            if (i++ >= data.size()) throw error::format(node, "Unexpected end.");
        }
    }
    static inline size_t skiptoEnd(JsonNode* node, const std::string_view& data, size_t& i)
    {
        while (true) {
            switch (data[i]) {
                case ' ':
                case ',':
                case '\t':
                case '\n':
                case '}':
                case ']': return i;
                default: break;
            }
            if (i++ >= data.size()) throw error::format(node, "Unexpected end.");
        }
    }
    static JsonNode* parse(const std::string_view& data, size_t& i)
    {
        JsonNode* rootNode = nullptr;
        try {
            skipSpaces(nullptr, data, i);
            if (data[i] == '{') {
                JsonObject* node = new JsonObject();
                i++;
                bool first = true;
                while (true) {
                    if (i >= data.size()) throw error::format(node, "Missing obj end }");
                    skipSpaces(node, data, i);
                    if (data[i] == '}') break;
                    if (first)
                        first = false;
                    else if (data[i++] != ',')
                        throw error::format(node, "Missing comma in object at", i, data.substr(i, 5));
                    skipSpaces(node, data, i);
                    if (data[i] != '\"')
                        throw error::format(node, "No begin quote before key at", i, data.substr(i, 5));
                    size_t entryKeyStart = i;
                    size_t entryKeyEnd   = data.find('\"', i + 1);
                    if (entryKeyEnd == std::string_view::npos)
                        throw error::format(node, "No begin quote after key at", i, data.substr(i, 5));
                    std::string_view entryKey =
                        data.substr(entryKeyStart + 1, entryKeyEnd - entryKeyStart - 1);
                    i = entryKeyEnd + 1;
                    skipSpaces(node, data, i);
                    if (data[i++] != ':')
                        throw error::format(node, "No color after key at", i, data.substr(i, 5));
                    JsonNode* entryNode = parse(data, i);
                    node->entries.insert({entryKey, entryNode});
                }
                i++;
                rootNode = node;
            }
            else if (data[i] == '[') {
                JsonArray* node = new JsonArray();
                i++;
                bool first = true;
                while (true) {
                    if (i >= data.size()) throw error::format(node, "Missing array end ]");
                    skipSpaces(node, data, i);
                    if (data[i] == ']') break;
                    if (first)
                        first = false;
                    else if (data[i++] != ',')
                        throw error::format(node, "Missing comma in array at", i, data.substr(i, 5));
                    skipSpaces(node, data, i);
                    node->elements.push_back(parse(data, i));
                }
                i++;
                rootNode = node;
            }
            else {
                JsonValue* node;
                size_t     start = i++, end;
                if (data[start] != '\"') {
                    end                    = skiptoEnd(node, data, i);
                    std::string_view value = data.substr(start, end - start);
                    if (value == "null")
                        node = new JsonNull();
                    else if (value == "true")
                        node = new JsonBool(true);
                    else if (value == "false")
                        node = new JsonBool(false);
                    else if (cW::is_digit(data[start]) || data[start] == '-')
                        node = new JsonNumber(data.data() + start);
                    else
                        throw error::format(nullptr, "Invalid non-string value at", i, data.substr(i, 5));
                }
                else {
                    end  = skipToStringEnd(node, data, i);
                    node = new JsonString(data.substr(start + 1, end - start - 1));
                    i++;
                }
                rootNode = node;
            }
        }
        catch (std::runtime_error& e) {
            if (rootNode) delete rootNode;
            throw e;
        }
        return rootNode;
    }
    static JsonNode* parse(const std::string_view& data)
    {
        size_t i = 0;
        return parse(data, i);
    }
};

} // namespace cW
#endif