#include "JsonValue.hpp"
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace json
{
    // Factory methods
    std::shared_ptr<JsonValue> JsonValue::null()
    {
        auto v = std::make_shared<JsonValue>();
        v->data = std::monostate{};
        return v;
    }

    std::shared_ptr<JsonValue> JsonValue::boolean(bool val)
    {
        auto v = std::make_shared<JsonValue>();
        v->data = val;
        return v;
    }

    std::shared_ptr<JsonValue> JsonValue::integer(int64_t val)
    {
        auto v = std::make_shared<JsonValue>();
        v->data = val;
        return v;
    }

    std::shared_ptr<JsonValue> JsonValue::floating(double val)
    {
        auto v = std::make_shared<JsonValue>();
        v->data = val;
        return v;
    }

    std::shared_ptr<JsonValue> JsonValue::string(const std::string& val)
    {
        auto v = std::make_shared<JsonValue>();
        v->data = val;
        return v;
    }

    std::shared_ptr<JsonValue> JsonValue::array()
    {
        auto v = std::make_shared<JsonValue>();
        v->data = JsonArray{};
        return v;
    }

    std::shared_ptr<JsonValue> JsonValue::object()
    {
        auto v = std::make_shared<JsonValue>();
        v->data = JsonObject{};
        return v;
    }

    // Type queries
    JsonType JsonValue::getType() const
    {
        switch (data.index())
        {
            case 0: return JsonType::NULL_VALUE;
            case 1: return JsonType::BOOLEAN;
            case 2: return JsonType::INTEGER;
            case 3: return JsonType::FLOAT;
            case 4: return JsonType::STRING;
            case 5: return JsonType::ARRAY;
            case 6: return JsonType::OBJECT;
            default: return JsonType::NULL_VALUE;
        }
    }

    bool JsonValue::isNull() const { return data.index() == 0; }
    bool JsonValue::isBool() const { return data.index() == 1; }
    bool JsonValue::isInt() const { return data.index() == 2; }
    bool JsonValue::isFloat() const { return data.index() == 3; }
    bool JsonValue::isNumber() const { return isInt() || isFloat(); }
    bool JsonValue::isString() const { return data.index() == 4; }
    bool JsonValue::isArray() const { return data.index() == 5; }
    bool JsonValue::isObject() const { return data.index() == 6; }

    // Accessors
    bool JsonValue::asBool() const
    {
        return std::get<bool>(data);
    }

    int64_t JsonValue::asInt() const
    {
        if (isFloat())
            return static_cast<int64_t>(std::get<double>(data));
        return std::get<int64_t>(data);
    }

    double JsonValue::asFloat() const
    {
        if (isInt())
            return static_cast<double>(std::get<int64_t>(data));
        return std::get<double>(data);
    }

    const std::string& JsonValue::asString() const
    {
        return std::get<std::string>(data);
    }

    const JsonValue::JsonArray& JsonValue::asArray() const
    {
        return std::get<JsonArray>(data);
    }

    const JsonValue::JsonObject& JsonValue::asObject() const
    {
        return std::get<JsonObject>(data);
    }

    // Mutators
    void JsonValue::addToArray(std::shared_ptr<JsonValue> item)
    {
        std::get<JsonArray>(data).push_back(std::move(item));
    }

    void JsonValue::setProperty(const std::string& key, std::shared_ptr<JsonValue> value)
    {
        std::get<JsonObject>(data)[key] = std::move(value);
    }

    bool JsonValue::hasProperty(const std::string& key) const
    {
        if (!isObject()) return false;
        const auto& obj = std::get<JsonObject>(data);
        return obj.find(key) != obj.end();
    }

    std::shared_ptr<JsonValue> JsonValue::getProperty(const std::string& key) const
    {
        const auto& obj = std::get<JsonObject>(data);
        auto it = obj.find(key);
        if (it != obj.end())
            return it->second;
        return nullptr;
    }

    // Serialization
    std::string JsonValue::toJsonString(bool pretty) const
    {
        std::string out;
        out.reserve(256);
        toJsonStringImpl(out, pretty, 0);
        return out;
    }

    void JsonValue::appendEscapedString(std::string& out, const std::string& str)
    {
        out += '"';
        for (char c : str)
        {
            switch (c)
            {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        out += buf;
                    }
                    else
                    {
                        out += c;
                    }
                    break;
            }
        }
        out += '"';
    }

    void JsonValue::appendIndent(std::string& out, int depth)
    {
        for (int i = 0; i < depth; ++i)
            out += "    ";
    }

    void JsonValue::toJsonStringImpl(std::string& out, bool pretty, int depth) const
    {
        switch (getType())
        {
            case JsonType::NULL_VALUE:
                out += "null";
                break;

            case JsonType::BOOLEAN:
                out += asBool() ? "true" : "false";
                break;

            case JsonType::INTEGER:
                out += std::to_string(asInt());
                break;

            case JsonType::FLOAT:
            {
                double val = asFloat();
                if (std::isinf(val) || std::isnan(val))
                {
                    out += "null";
                }
                else
                {
                    std::ostringstream oss;
                    oss << std::setprecision(17) << val;
                    std::string s = oss.str();
                    // Ensure it has a decimal point so it parses as float
                    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos)
                        s += ".0";
                    out += s;
                }
                break;
            }

            case JsonType::STRING:
                appendEscapedString(out, asString());
                break;

            case JsonType::ARRAY:
                toJsonStringArray(out, pretty, depth);
                break;

            case JsonType::OBJECT:
                toJsonStringObject(out, pretty, depth);
                break;
        }
    }

    void JsonValue::toJsonStringArray(std::string& out, bool pretty, int depth) const
    {
        const auto& arr = asArray();
        if (arr.empty())
        {
            out += "[]";
            return;
        }

        out += '[';
        for (size_t i = 0; i < arr.size(); ++i)
        {
            if (i > 0) out += ',';
            if (pretty)
            {
                out += '\n';
                appendIndent(out, depth + 1);
            }
            if (arr[i])
                arr[i]->toJsonStringImpl(out, pretty, depth + 1);
            else
                out += "null";
        }
        if (pretty)
        {
            out += '\n';
            appendIndent(out, depth);
        }
        out += ']';
    }

    void JsonValue::toJsonStringObject(std::string& out, bool pretty, int depth) const
    {
        const auto& obj = asObject();
        if (obj.empty())
        {
            out += "{}";
            return;
        }

        out += '{';
        bool first = true;

        // Output __type first if present for readability
        auto typeIt = obj.find("__type");
        if (typeIt != obj.end())
        {
            if (pretty) { out += '\n'; appendIndent(out, depth + 1); }
            appendEscapedString(out, "__type");
            out += pretty ? ": " : ":";
            typeIt->second->toJsonStringImpl(out, pretty, depth + 1);
            first = false;
        }

        for (const auto& [key, val] : obj)
        {
            if (key == "__type") continue;
            if (!first) out += ',';
            if (pretty) { out += '\n'; appendIndent(out, depth + 1); }
            appendEscapedString(out, key);
            out += pretty ? ": " : ":";
            if (val)
                val->toJsonStringImpl(out, pretty, depth + 1);
            else
                out += "null";
            first = false;
        }
        if (pretty)
        {
            out += '\n';
            appendIndent(out, depth);
        }
        out += '}';
    }
}
