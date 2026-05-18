#include "UserException.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/ValueShim.hpp"

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
        if (value::isObject(exceptionValue))
        {
            auto objInstance = value::asObject(exceptionValue);
            if (objInstance)
            {
                return objInstance->isInstanceOf(catchType);
            }
        }

        return false;
    }
}
