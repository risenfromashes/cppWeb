#ifndef __CW_JSON_H_
#define __CW_JSON_H_

#include "Utils.h"
#include <sstream>
#include <unordered_map>

namespace cW {
// clang-format off
#ifdef __cpp_nontype_template_parameter_class

template<FixedString prefix>
class RuntimeError : public std::runtime_error {
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

    std::string errorString;

  public:
    RuntimeError(const std::string& errorString)
        : std::runtime_error(prefix), errorString(errorString)
    {
    }
    template <typename P, typename... R>
    static inline RuntimeError format(P* toDelete, const std::string& message, R... args)
    {
        if (toDelete) delete toDelete;
        std::ostringstream oss;
        oss << prefix << message;
        if constexpr (sizeof...(args) > 0) __write(oss, args...);
        return RuntimeError(oss.str());
    }
    template <typename... R>
    static inline void format(const std::string& message, R... args)
    {
        return format(nullptr, message, args...);
    }
    const char* what() const noexcept override { return errorString.c_str(); }
};
#endif

#ifdef __cpp_concepts
// clang-format off

struct JsonNode;

template <typename T, class _Class_T>
concept __JsonEntry = requires(std::ostream& os, _Class_T* ptr, JsonNode* node){
    { T::key }->std::convertible_to<const char*>;
    T::write(ptr, os);
    T::set(ptr, node);
    //{T::toJson(ptr) }->std::convertible_to<std::string>;
};

template <typename T>
concept __JsonVal = std::is_convertible_v<T, const char*> && !std::is_unbounded_array_v<T> ||
                     (!std::is_array_v<clean_t<T>> &&
                      (requires(clean_t<T> a, const char* v, size_t l) { (decltype(a))(v, l); } || 
                       requires(clean_t<T> a, const std::string& v) { (decltype(a))(v); } || 
                       requires(clean_t<T> a, const std::string_view& v) { (decltype(a))(v); } || //view of json data
                       requires(clean_t<T> a, int v) { (decltype(a))(v); } || // number or bool
                       requires(clean_t<T> a, JsonNode* v) { a(v); } || // object
                       requires(clean_t<T> a, JsonNode* v) { (decltype(a))::parseJson(v); } // object
                      ) &&
                      (requires(clean_t<T> val, std::ostream& os) { os << val; } ||
                       requires(clean_t<T> val, std::ostream& os) { val.writeJson(os); } ||
                       requires(clean_t<T> val) { { val.toString()}->std::convertible_to<std::string>; } ||
                       requires(clean_t<T> val) { { val.toJson()}->std::convertible_to<std::string>; }
                      )
                     );


template <typename _Container_T>
concept __JsonContainer = requires(_Container_T a)
                          {
                              _Container_T();
                              a.begin();
                              ++(a.begin());
                              a.end();
                              a.clean();
                              a.size();
                              requires requires(clean_t<decltype(*a.begin())> t)
                              {
                                  requires requires { a.push_back(t); }
                                  || requires { a.insert(t); };
                              };
                          };

// upto 3D array
template <typename _Container_T>
concept __JsonArray = (std::is_bounded_array_v<clean_t<_Container_T>> &&
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
                            requires(clean_t<decltype(*a.begin())> _a){
                                requires __JsonContainer<decltype(_a)>;
                                requires __JsonVal<clean_t<decltype(*_a.begin())>> ||
                                requires(clean_t<decltype(*_a.begin())> __a){
                                  requires __JsonContainer<decltype(__a)>;
                                  requires __JsonVal<clean_t<decltype(*__a.begin())>>;
                                };
                           };
                        };


//return type cannot even be char[N]
template <typename T>
concept __JsonReturnVal = requires __JsonVal<T> && !std::is_array_v<T> && !is_ref_v<T> &&
                          (std::is_pointer_v<T> || std::is_copy_constructible_v<clean_t<T>>) || //parsable
                          std::is_same_v<cW::clean_t<T>, JsonNode> ||
                          std::is_base_of_v<JsonNode, cW::clean_t<T>>; //node value
template <typename T>
concept __JsonReturnArray_ = requires(T a, size_t n){ a.size(); a.resize(n); };

template <typename T>
concept __JsonReturnArray = !is_ref_v<T> && 
                           (std::is_convertible_v<std::vector<JsonNode*>, T> ||
                            requires(T a){ requires
                           __JsonReturnArray_<decltype(a)>       && __JsonReturnVal<cW::clean_t<decltype(a[0])>> 
                        || __JsonReturnArray_<decltype(a[0])>    && __JsonReturnVal<cW::clean_t<decltype(a[0][0])>> 
                        || __JsonReturnArray_<decltype(a[0][0])> && __JsonReturnVal<cW::clean_t<decltype(a[0][0][0])>>; });

#endif



struct JsonNode {
#ifdef __cpp_nontype_template_parameter_class
    using error = RuntimeError<"Invalid Json path.">;
#endif
    enum Type { Value, Array, Object };
    const Type type;
    virtual ~JsonNode() {}
    template <typename T = long double>
#ifdef __cpp_concepts
        requires __JsonReturnVal<T> || __JsonReturnArray<T> 
#endif
    T get(const std::string_view& path);
  protected:
    JsonNode(Type type) noexcept : type(type) {}
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
    JsonString(const char* data, size_t len) : JsonValue(JsonValue::String), value(data, len) {}
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
    std::unordered_map<std::string_view, JsonNode*> entries;
    JsonObject() : JsonNode{JsonNode::Object} {}
    ~JsonObject() override
    {
        for (auto [key, node] : entries)
            delete node;
        entries.clear();
    }
};

#ifdef __cpp_nontype_template_parameter_class

template <FixedString _key, auto _member_ptr>
    requires std::is_member_pointer_v<decltype(_member_ptr)> &&
     (__JsonVal<member_pointer_member_t<_member_ptr>> || __JsonArray<member_pointer_member_t<_member_ptr>>) 
struct JsonEntry {
    using error = RuntimeError<"Invalid conversion from json to object.">;
    using _Class_T = member_pointer_class_t<_member_ptr>;
    using _Member_T = member_pointer_member_t<_member_ptr>;
    using _Clean_Tp = std::remove_cv_t<_Member_T>;
    using _Clean_T = clean_t<_Member_T>;

    static constexpr const char* key        = _key;
    static constexpr const bool  is_pointer = std::is_pointer_v<_Member_T>;
    
    static inline const _Member_T& get(const _Class_T* _this)
    {
        return _this->*_member_ptr;
    }

    static inline auto getPtr(_Class_T* _this)
    {
        if constexpr(is_pointer) return const_cast<_Clean_T**>(&(_this->*_member_ptr));
        else return const_cast<_Clean_T*>(&(_this->*_member_ptr));
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
    static auto getRef(T& val)
        -> std::add_lvalue_reference_t<std::remove_pointer_t<T>>
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

    template <typename _Array_T>
    static void writeArray(const _Array_T& arr, std::ostream& os)
    {
        os << "[";
        using Tp             = std::remove_cvref_t<decltype(arr[0])>;
        using T              = clean_t<Tp>;
        constexpr bool isPtr = std::is_pointer_v<Tp>;
        for (int i = 0; i < std::extent_v<_Array_T>; i++) {
            const T& valp = arr[i];
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
            const T& val = getRef(valp);
            if constexpr (level < rank - 1)
                writeArray<T>(val, os);
            else
                writeVal(val, os);
            if (i < std::extent_v<_Array_T> - 1) os << ",";
        }
        os << "]";
    }

    template<typename _Array_T>
    static void setArray(_Array_T& arr, JsonArray* jarr){
        using Tp = std::remover_cvref_t<decltype(arr[0])>;
        using T  = clean_t<Tp>;
        constexpr bool isPtr = std::is_pointer_v<Tp>;
        size_t len = std::min(jarr->elements.size(), std::extent_v<_Array_T>);
        for(int i = 0; i < len; i++){
            if constexpr(__JsonReturnVal<T>)
                arr[i] = jarr->elements[i].get<Tp>();
            else {
                if(jarr->elements[i]->type == JsonNode::Array)
                    setArray<T>(arr[i], (JsonArray*)jarr->elements[i]);
                else throw error::format("Failed conversion from json array.");
            }
        }
    }

    //template <typenaem _Array_T, unsigned int rank, unsigned int
    template <typename _List_T>
    static void writeList(const _List_T& arr, std::ostream& os)
    {
        os << "[";
        using Tp             = std::remove_cvref_t<decltype(*arr.begin())>;
        using T              = clean_t<Tp>;
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
            const T& val = getRef(valp);
            if constexpr (__JsonVal<T>)
                writeVal(val, os);
            else
                writeList<T>(val, os);
            if (++i < size) os << ",";
        }
        os << "]";
    }

    template<typename _List_T>
    static void setList(_List_T& arr, JsonArray* jarr){
        using Tp = std::remove_cvref_t<decltype(*arr.begin())>;
        using T  = clean_t<Tp>;
        constexpr bool isPtr = std::is_pointer_v<Tp>;
        size_t len = std::min(jarr->elements.size(), std::extent_v<_Array_T>);
        if (arr.size() > 0) arr.clear();
        for(int i = 0; i < len; i++){
            if constexpr(__JsonReturnVal<T>){
                if constexpr(requires(_List_T a, Tp b){ a.push_back(b)})
                    arr.push_back(jarr->elements[i].get<Tp>());
                else
                    arr.insert(jarr->elements[i].get<Tp>());
            }
            else {
                if(jarr->elements[i]->type == JsonNode::Array){
                    auto elm = createInstance<T, isPtr>();
                    if constexpr(requires(_List_T a, Tp b){ a.push_back(b)})
                        arr.push_back(elm);
                    else
                        arr.insert(elm);
                    setList<T>(getRef(elm), (JsonArray*)jarr->elements[i]);
                }
                else throw error::format("Failed conversion from json array.");
            }
        }
    }

    static void write(const _Class_T* _this, std::ostream& os)
    {
        using Tp = std::remove_cv_t<_Member_T>;
        using T  = _Clean_T;
        os << '\"' << key << "\": ";
        const Tp& valp = get(_this);
        if constexpr (is_pointer) {
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
        else
            writeList<T>(val, os);
    }
    template <bool useDtor>
    static void setVal(_Clean_T* ptr){
        using T = _Clean_T;
        if constexpr (is_pointer)
            *ptr = new T();
        else {
            if constexpr(useDtor && requires { T::~T(); }) ptr->~T();
            new(ptr) T();
        }
    }
    template <bool useDtor, typename... R>
    static void setVal(_Clean_T* ptr, R&&... args){
        using T = _Clean_T;
        if constexpr (is_pointer)
            *ptr = new T(std::forward<R>(args)...);
        else {
            if constexpr(useDtor && requires { T::~T(); }) ptr->~T();
            new(ptr) T(std::forward<R>(args)...);
        }
    }
    template <bool useCtor>
    static void setVal(_Clean_T* ptr, JsonObject* jobj){
        using T = _Clean_T;
        if constexpr (is_pointer){
            if constexpr(useCtor)
                *ptr = new T(jobj);
            else
                *ptr = T::parseJson<true>(jobj);
        }
        else {
            if constexpr(requires { T::~T(); }) ptr->~T();
            if constexpr(useCtor)
                new(ptr) T(jobj);
            else
                T::parseJson(jobj, ptr);
        }
    }

    template<bool useDtor>
    static void set(_Class_T* _this, JsonNode* node)
    {
        using Tp = std::remove_cv_t<_Member_t>;
        using T  = _Clean_T;
        T* ptr = getPtr(_this);
        switch (node->type) {
            case JsonNode::Value: {
                switch (((JsonValue*)node)->valueType) {
                    case JsonValue::Bool: {
                        auto jbool = static_cast<JsonBool*>(node);
                        if constexpr (requires(bool v){ T(v); })
                            setVal<useDtor>(ptr, jbool->value);
                        else  throw error::format("Member", key, "is not constructible from bool, found in json entry.");
                        break;
                    }
                    case JsonValue::Number: {
                        auto jnumber = static_cast<JsonNumber*>(node);
                        if constexpr (requires(int v){ T(v); }){
                            if constexpr(std::is_arithmetic_v<T>)
                                setVal<useDtor>(ptr, jnumber->get<T>());
                            else
                                setVal<useDtor>(ptr, jnumber->get()); //as long double; default
                        }
                        else  throw error::format("Member", key, "is not constructible from number, found in json entry.");
                        break;
                    }
                    case JsonValue::Null: {
                        if constexpr (is_pointer) *ptr = nullptr;
                        else throw error::format("Member", key, "is not a pointer. But found null in json entry");
                        break;
                    }
                    case JsonValue::String: {
                        auto jstr = static_cast<JsonString*>(node);
                        size_t len = jstr->value.size();
                        if constexpr(std::is_same_v<T, char>){
                            if constexpr(is_pointer){
                                size_t len = jstr->value.data();
                                char* cpy = (char*)malloc(len + 1);
                                std::memcpy(cpy, jstr->value.data(), len);
                                *(cpy + len) = '\0';
                                *ptr = cpy;
                            }
                            else// must be a char at this point
                                *ptr = jstr->value.empty() ? '\n' : jstr->value[0];
                        }
                        if constexpr(std::is_bounded_array_v<T> && requires(T a){ requires std::is_same_v<std::remove_cvref_t<decltype(a[0])>>; }){
                            len = std::min(len, std::extent_v<T>);
                            for(size_t i = 0; i < len; i++)
                                (*ptr)[i] = jstr->value[i];
                        }
                        else if constexpr(requires(const char* v, size_t len){ T(v, len);})
                            setVal<useDtor>(ptr, jstr->value.data(), jstr->value.size());
                        else if constexpr(requires(const std::string& v){ T(v); })
                            setVal<useDtor>(ptr, std::string(jstr->value));   
                        else if constexpr(requires(const std::string_view& v){ T(v); })
                            setVal<useDtor>(ptr, jstr->value);
                        else throw error::format("Member", key, "is not constructible from string value found in json entry.");
                        break;
                    }
                }
                break;
            }
            case JsonNode::Object:{
                auto jobj = static_cast<JsonObject*>(node);
                if constexpr (useDtor && requires (JsonNode* a){ T(a); })
                    setVal<true>(ptr, jobj);                 
                else if constexpr (requires (JsonNode* a){ T::parseJson(a); })
                    setVal<false>(ptr, jobj);
                else throw error::format("Member", key, "is not parsable from json object.");
                break;
            }
            case JsonNode::Array:{
                auto jarr = static_cast<JsonArray*>(node);
                if constexpr(std::is_bounded_array_v<T>)
                    setArray<T>(*ptr, jarr);
                else if constexpr (__JsonArray<T>){
                    setVal<useDtor>(ptr);
                    setList<T>(getRef(*ptr), jarr);
                }   
                else throw error::format("Member", key, "is not constructible from array found in json entry.");    
                break;
            }
        }
    }
};

// clang-format on

template <class _Class_T, __JsonEntry<_Class_T>... _Entries>
struct JsonContext {
    using error = RuntimeError<"Invalid json node.">;
    // for stack allocation
    union U {
        char     c;
        char     buf[sizeof(_Class_T)];
        _Class_T obj;
        U() : c(0) {}
        ~U()
        {
            if constexpr (requires(_Class_T a) { a.~_Class_T(); }) obj.~_Class_T();
        }
    };

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
    template <size_t                index,
              bool                  useDtor,
              __JsonEntry<_Class_T> _entry,
              __JsonEntry<_Class_T>... _entries>
    static void _set(_Class_T* ptr, JsonObject* jsonObject)
    {
        auto itr = jsonObject->entries.find(_entry::key);
        if (itr != jsonObject->entries.end()) _entry::set(ptr, itr->second);
        if constexpr (index > 0) _set<index - 1, useDtor, _entries...>(ptr, jsonObject);
    }
    static void write(const _Class_T* ptr, std::ostream& os)
    {
        os << "{";
        _write<entry_count - 1, _Entries...>(ptr, os);
        os << "}";
    }
    // clang-format off
    template<bool pointer = false>
    static auto construct(JsonNode* jsonNode)
    {
        if(jsonNode->type != JsonNode::Object) throw error::format("Cannot construct object from non-object json node");
        if constexpr(pointer){
            static std::allocator<_Class_T> allocator;
            _Class_T* ptr = allocator.allocate(1);
            //allocating ourselfves, so no dtor needed
            _set<entry_count - 1, false, _Entries...>(ptr, (JsonObject*)jsonNode);
            return ptr;
        }
        else{
            U u;
            _set<entry_count - 1, false, _Entries...>(&u.obj, (JsonObject*)jsonNode);
            return u.obj;
        }
    }
    template<bool useDtor = true>
    static void construct(JsonNode* jsonNode, _Class_T* ptr)
    {
        if(jsonNode->type != JsonNode::Object) throw error::format("Cannot construct object from non-object json node");
        _set<entry_count - 1, useDtor, _Entries...>(ptr, (JsonObject*)jsonNode);
    }
};
// clang-format on

#define JSONENTRY(type, mem) , cW::JsonEntry<#mem, &type::mem>
#define JSONABLE(type, ...)                                                                     \
    using JsonCtx = cW::JsonContext<type CW_MACRO_FOR_EACH_EX(JSONENTRY, type, __VA_ARGS__)>;   \
    friend JsonCtx;                                                                             \
    static inline auto parseJson(JsonNode* jsonNode)                                            \
    {                                                                                           \
        if constexpr (std::is_copy_constructible_v<T>)                                          \
            return JsonCtx::construct(jsonNode);                                                \
        else                                                                                    \
            return JsonCtx::construct<true /*Pointer*/>(jsonNode);                              \
    }                                                                                           \
    static inline void parseJson(JsonNode* jsonNode, type* _this)                               \
    {                                                                                           \
        JsonCtx::construct(jsonNode, _this);                                                    \
    }                                                                                           \
    static inline type* fromJson(JsonNode* jsonNode)                                            \
    {                                                                                           \
        return JsonCtx::construct<true /*Pointer*/>(jsonNode);                                  \
    }                                                                                           \
    inline void writeJson(std::ostream& os = std::cout) const { JsonContext::write(this, os); } \
    inline std::string toJson() const                                                           \
    {                                                                                           \
        std::ostringstream oss;                                                                 \
        writeJson(oss);                                                                         \
        return oss.str();                                                                       \
    }
#else

struct JsonContext;
#define JSONABLE(type, ...)                                            \
  public:                                                              \
    static inline type  parseJson(JsonNode* jsonNode);                 \
    static inline type  parseJson(JsonNode* jsonNode, type* _this);    \
    static inline type* fromJson(JsonNode* jsonNode);                  \
    inline void         writeJson(std::ostream& os = std::cout) const; \
    inline std::string  toJson() const;

#endif

struct JsonParser {

#ifdef __cpp_nontype_template_parameter_class
    using error = RuntimeError<"Invalid Json.">;
#endif;
    JsonNode* rootNode = nullptr;
    char*     readBuf  = nullptr;
    JsonParser(const char* data, size_t length);
    JsonParser(const std::string_view& data);
    JsonParser(std::istream& in);
    ~JsonParser();
    static inline size_t
    skipSpaces(JsonNode* node, const char* data, const size_t length, size_t& i);
    static inline size_t
    skipToStringEnd(JsonNode* node, const char* data, const size_t length, size_t& i);
    static inline size_t
                     skiptoEnd(JsonNode* node, const char* data, const size_t length, size_t& i);
    static JsonNode* parse(const char* data, const size_t length, size_t& i);
    static JsonNode* parse(const std::string_view& data);
};

#ifdef __cpp_nontype_template_parameter_class
size_t JsonParser::skipSpaces(JsonNode* node, const char* data, const size_t length, size_t& i)
{
    while (true) {
        switch (data[i]) {
            case ' ':
            case '\t':
            case '\n': break;
            default: return i;
        }
        if (++i >= length) throw error::format(node, "Unexpected end.");
    }
    return i;
}
size_t JsonParser::skipToStringEnd(JsonNode* node, const char* data, const size_t length, size_t& i)
{
    bool slash = false;
    while (true) {
        if (data[i] == '\"' && !slash) return i;
        slash = data[i] == '\\';
        if (++i >= length) throw error::format(node, "Unexpected end.");
    }
}
size_t JsonParser::skiptoEnd(JsonNode* node, const char* data, const size_t length, size_t& i)
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
        if (++i >= length) throw error::format(node, "Unexpected end.");
    }
}

// clang-format off
template <typename _Ret_T>
    requires __JsonReturnVal<_Ret_T> || __JsonReturnArray<_Ret_T> 
_Ret_T JsonNode::get(const std::string_view& path)
{
    using T = clean_t<_Ret_T>;
    constexpr bool is_pointer = std::is_pointer_v<_Ret_T>;
    if (path.empty()) {
        if constexpr (std::is_same_v<T, JsonNode> ||
                      std::is_base_of_v<JsonNode, T>) {
            if constexpr (is_pointer)
                return (_Ret_T)this;
            else
                return *((_Ret_T*)this);
        }
        switch(type){
            case Value: {
                if constexpr(__JsonVal<T>){
                    switch(((JsonValue*)this)->valueType){
                        case JsonValue::Bool: {
                            auto jbool = static_cast<JsonBool*>(this);
                            if constexpr (requires(bool v){ T(v); })
                                return createInstance<T, is_pointer>(jbool->value);
                            else  throw error::format("Return type is not constructible from bool type found in json entry.");
                            break;
                        }
                        case JsonValue::Number: {
                            auto jnumber = static_cast<JsonNumber*>(this);
                            if constexpr (requires(int v){ T(v); }){
                                if constexpr(std::is_arithmetic_v<T>)
                                    return createInstance<T, is_pointer>(jnumber->get<T>());
                                else
                                    return createInstance<T, is_pointer>(jnumber->get()); //as long double; default
                            }
                            else  throw error::format("Return type is not constructible from number type found in json entry.");
                        }
                        case JsonValue::Null: {
                            if constexpr (is_pointer) return nullptr;
                            else throw error::format("Return type is not a pointer. But found null in json entry,");
                        }
                        case JsonValue::String: {
                            auto jstr = static_cast<JsonString*>(this);
                            size_t len = jstr->value.data();
                            if constexpr(std::is_same_v<T, char>){
                                if constexpr(is_pointer){
                                    size_t len = jstr->value.size();
                                    char* cpy = (char*)malloc(len + 1);
                                    std::memcpy(cpy, jstr->value.data(), len);
                                    *(cpy + len) = '\0';
                                    return cpy;
                                }
                                else// must be a char at this point
                                    return jstr->value.empty() ? '\n' : jstr->value[0];
                            }
                            else if constexpr(requires(const char* v, size_t len){ T(v, len);})
                                return createInstance<T, is_pointer>(jstr->value.data(), jstr->value.size());
                            else if constexpr(requires(const std::string& v){ T(v); })
                                return createInstance<T, is_pointer>(std::string(jstr->value));   
                            else if constexpr(requires(const std::string_view& v){ T(v); })
                                return createInstance<T, is_pointer>(jstr->value);
                            else throw error::format("Return type is not constructible from string value found in json.");
                        }
                    }
                }
                throw error::format("Invalid return type");
            }
            case Object: {
                if constexpr(requires(JsonNode* node){ T::parseJson(node); } && !is_pointer)
                    return T::parseJson(this);
                else if constexpr(requires(JsonNode* node){ T::fromJson(node); })
                    return T::fromJson(this);
                else throw error::format("Return type cannot be parsed from json object.");
            }
            case Array: {
                static_assert(!is_pointer, "Pointer to array return not allowed");
                JsonArray* node = static_cast<JsonArray*>(this);
                if constexpr (std::is_convertible_v<std::vector<JsonNode*>, _Ret_T>)
                    return node->elements;
                else if constexpr (__JsonReturnArray<T>) {
                    size_t len = node->elements.size();
                    arr.resize(len);
                    for (size_t i = 0; i < len; i++) {
                        arr[i] = node->elements[i]->get<std::remove_cvref_t<decltype(arr[0])>>("");
                    }
                    return arr;
                }
                throw error::format("Asking for invalid value on array entry");
            }
        }
        throw error::format("Asking for invalid value");
    }
    if (path[0] != '/') throw std::runtime_error("Invalid path");
    size_t           slash    = path.find('/', 1);
    std::string_view key      = path.substr(1, slash - 1);
    std::string_view nextPath = slash < std::string_view::npos ? path.substr(slash) : "";
    if (type == Object) {
        JsonObject* node = static_cast<JsonObject*>(this);
        auto        itr  = node->entries.find(key);
        if (itr == node->entries.end()) { throw error::format("Entry", key, " not found"); }
        return itr->second->get<_Ret_T>(nextPath);
    }
    if (type == Array) {
        JsonArray*  node   = static_cast<JsonArray*>(this);
        size_t      keyLen = key.size();
        char*       end;
        const char* start = key.data();
        size_t      index = strtoull(start, &end, 10);
        if (end - start < keyLen) throw error::format("Invalid index on array node");
        if (index >= node->elements.size()) throw error::format("Array index Out of Bound");
        return node->elements[index]->get<_Ret_T>(nextPath);
    }
    throw error::format("Invalid path");
}


#endif
} // namespace cW
#endif