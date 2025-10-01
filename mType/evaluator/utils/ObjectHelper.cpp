#include "ObjectHelper.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include <variant>

namespace evaluator {
namespace utils {

    using namespace runtimeTypes::klass;

    std::string ObjectHelper::getClassName(const Value& objectValue)
    {
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto obj = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            if (obj)
            {
                return obj->getClassDefinition()->getName();
            }
        }
        return "unknown";
    }

    bool ObjectHelper::isInstanceOf(const Value& objectValue, const std::string& className)
    {
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto obj = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            if (obj)
            {
                std::string actualClassName = obj->getClassDefinition()->getName();
                return actualClassName == className;
            }
        }
        return false;
    }

    bool ObjectHelper::areObjectTypesCompatible(const Value& expected, const Value& actual)
    {
        // Both values should be objects
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(expected) ||
            !std::holds_alternative<std::shared_ptr<ObjectInstance>>(actual))
        {
            return false;
        }

        auto expectedObj = std::get<std::shared_ptr<ObjectInstance>>(expected);
        auto actualObj = std::get<std::shared_ptr<ObjectInstance>>(actual);

        if (!expectedObj || !actualObj)
        {
            return false;
        }

        // Compare class names
        std::string expectedClassName = expectedObj->getClassDefinition()->getName();
        std::string actualClassName = actualObj->getClassDefinition()->getName();

        return expectedClassName == actualClassName;
    }

    std::shared_ptr<ObjectInstance> ObjectHelper::extractInstance(const Value& value)
    {
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(value))
        {
            return std::get<std::shared_ptr<ObjectInstance>>(value);
        }
        return nullptr;
    }

} // namespace utils
} // namespace evaluator
