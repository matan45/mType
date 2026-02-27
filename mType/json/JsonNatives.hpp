#pragma once
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include <memory>
#include <vector>

namespace json
{
    class JsonNatives
    {
    public:
        static void registerAll(std::shared_ptr<environment::Environment> env);
        static void cleanup();

    private:
        static value::Value __json_serialize(const std::vector<value::Value>& args);
        static value::Value __json_serializeWithOptions(const std::vector<value::Value>& args);
        static value::Value __json_deserialize(const std::vector<value::Value>& args);
        static value::Value __json_deserializeAs(const std::vector<value::Value>& args);
        static value::Value __json_format(const std::vector<value::Value>& args);
        static value::Value __json_stringify(const std::vector<value::Value>& args);
        static value::Value __json_readFile(const std::vector<value::Value>& args);
        static value::Value __json_writeFile(const std::vector<value::Value>& args);

        static void validateArgCount(const std::vector<value::Value>& args,
                                     size_t expected, const std::string& funcName);
        static std::string extractString(const value::Value& arg,
                                         const std::string& funcName);
        static bool extractBool(const value::Value& arg,
                                const std::string& funcName);

        static std::shared_ptr<environment::Environment> currentEnvironment;
    };
}
