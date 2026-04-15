#pragma once
#include "../environment/Environment.hpp"
#include "../value/ValueType.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace net
{
    // Convert an mType HashMap<string,string> instance to a std::unordered_map.
    // Returns an empty map if the value is null / not a HashMap.
    std::unordered_map<std::string, std::string>
    hashMapToStdMap(const value::Value& v);

    // Build an mType HashMap<string,string> instance from a std::unordered_map.
    // Mirrors HashMap.mt's bucket layout (see JsonDeserializer.cpp:525).
    value::Value stdMapToHashMap(std::shared_ptr<environment::Environment> env,
                                 const std::unordered_map<std::string, std::string>& map);
}
