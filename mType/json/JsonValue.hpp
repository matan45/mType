#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <stdexcept>

namespace json
{
    enum class JsonType : uint8_t
    {
        NULL_VALUE,
        BOOLEAN,
        INTEGER,
        FLOAT,
        STRING,
        ARRAY,
        OBJECT
    };

    class JsonValue
    {
    public:
        using JsonObject = std::map<std::string, std::shared_ptr<JsonValue>>;
        using JsonArray = std::vector<std::shared_ptr<JsonValue>>;

    private:
        std::variant<
            std::monostate,
            bool,
            int64_t,
            double,
            std::string,
            JsonArray,
            JsonObject
        > data;

    public:
        // Factory methods
        static std::shared_ptr<JsonValue> null();
        static std::shared_ptr<JsonValue> boolean(bool v);
        static std::shared_ptr<JsonValue> integer(int64_t v);
        static std::shared_ptr<JsonValue> floating(double v);
        static std::shared_ptr<JsonValue> string(const std::string& v);
        static std::shared_ptr<JsonValue> array();
        static std::shared_ptr<JsonValue> object();

        // Type queries
        JsonType getType() const;
        bool isNull() const;
        bool isBool() const;
        bool isInt() const;
        bool isFloat() const;
        bool isNumber() const;
        bool isString() const;
        bool isArray() const;
        bool isObject() const;

        // Accessors
        bool asBool() const;
        int64_t asInt() const;
        double asFloat() const;
        const std::string& asString() const;
        const JsonArray& asArray() const;
        const JsonObject& asObject() const;

        // Mutators for building
        void addToArray(std::shared_ptr<JsonValue> item);
        void setProperty(const std::string& key, std::shared_ptr<JsonValue> value);
        bool hasProperty(const std::string& key) const;
        std::shared_ptr<JsonValue> getProperty(const std::string& key) const;

        // Serialization to JSON string
        std::string toJsonString(bool pretty = false) const;

    private:
        void toJsonStringImpl(std::string& out, bool pretty, int depth) const;
        void toJsonStringArray(std::string& out, bool pretty, int depth) const;
        void toJsonStringObject(std::string& out, bool pretty, int depth) const;
        static void appendEscapedString(std::string& out, const std::string& str);
        static void appendIndent(std::string& out, int depth);
    };
}
