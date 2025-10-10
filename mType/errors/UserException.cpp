#include "UserException.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace errors
{
    bool UserException::matchesCatchType(const std::string& catchType) const
    {
        // Exact match
        if (exceptionTypeName == catchType)
        {
            return true;
        }

        // Check inheritance: if exception is an ObjectInstance, use isInstanceOf()
        // This properly handles exception hierarchies (e.g., NullPointerException extends Exception)
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue))
        {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue);
            if (objInstance)
            {
                return objInstance->isInstanceOf(catchType);
            }
        }

        return false;
    }
}
