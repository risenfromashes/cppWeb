#include "Json.h"

namespace cW {

#ifdef __cpp_nontype_template_parameter_class

JsonNode* JsonParser::parse(const char* data, const size_t length, size_t& i)
{
    JsonNode* rootNode = nullptr;
    try {
        skipSpaces(nullptr, data, length, i);
        if (data[i] == '{') {
            JsonObject* node = new JsonObject();
            i++;
            bool first = true;
            while (true) {
                if (i >= length) throw error::formatAndDelete(node, "Missing obj end }");
                skipSpaces(node, data, length, i);
                if (data[i] == '}') break;
                if (first)
                    first = false;
                else if (data[i++] != ',')
                    throw error::formatAndDelete(
                        node, "Missing comma in object at", i, std::string_view(data + i, 5));
                skipSpaces(node, data, length, i);
                if (data[i] != '\"')
                    throw error::formatAndDelete(
                        node, "No begin quote before key at", i, std::string_view(data + i, 5));
                size_t entryKeyStart = i;
                size_t entryKeyEnd   = i + 1;
                while (data[entryKeyEnd] != '\"')
                    if (++entryKeyEnd >= length)
                        throw error::formatAndDelete(
                            node, "No begin quote after key at", i, std::string_view(data + i, 5));
                std::string_view entryKey =
                    std::string_view(data + entryKeyStart + 1, entryKeyEnd - entryKeyStart - 1);
                i = entryKeyEnd + 1;
                skipSpaces(node, data, length, i);
                if (data[i++] != ':')
                    throw error::formatAndDelete(
                        node, "No color after key at", i, std::string_view(data + i, 5));
                JsonNode* entryNode = parse(data, length, i);
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
                if (i >= length) throw error::formatAndDelete(node, "Missing array end ]");
                skipSpaces(node, data, length, i);
                if (data[i] == ']') break;
                if (first)
                    first = false;
                else if (data[i++] != ',')
                    throw error::formatAndDelete(
                        node, "Missing comma in array at", i, std::string_view(data + i, 5));
                skipSpaces(node, data, length, i);
                node->elements.push_back(parse(data, length, i));
            }
            i++;
            rootNode = node;
        }
        else {
            JsonValue* node;
            size_t     start = i++, end;
            if (data[start] != '\"') {
                end = skiptoEnd(node, data, length, i);
                if (!std::memcmp(data + start, "null", 4))
                    node = new JsonNull();
                else if (!std::memcmp(data + start, "true", 4))
                    node = new JsonBool(true);
                else if (!std::memcmp(data + start, "false", 5))
                    node = new JsonBool(false);
                else if (cW::is_digit(data[start]) || data[start] == '-')
                    node = new JsonNumber(data + start);
                else
                    throw error::format(
                        "Invalid non-string value at", i, std::string_view(data + i, 5));
            }
            else {
                end  = skipToStringEnd(node, data, length, i);
                node = new JsonString(data + start + 1, end - start - 1);
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
JsonNode* JsonParser::parse(const std::string_view& data)
{
    size_t i = 0;
    return parse(data.data(), data.size(), i);
}

JsonParser::JsonParser(const char* data, size_t length)
{
    size_t i = 0;
    rootNode = parse(data, length, i);
}

JsonParser::JsonParser(const std::string_view& data)
{
    size_t i = 0;
    rootNode = parse(data.data(), data.size(), i);
}

JsonParser::JsonParser(std::istream& in)
{
    in.seekg(0, in.end);
    size_t size = in.tellg();
    in.seekg(0, in.beg);
    readBuf = (char*)malloc(size);
    in.read(readBuf, size);
    size_t i = 0;
    rootNode = parse(readBuf, size, i);
}

JsonParser::~JsonParser()
{
    if (rootNode) delete rootNode;
    if (readBuf) free(readBuf);
}

#endif

}; // namespace cW