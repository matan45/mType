#include "DependencyTypeUtils.hpp"

namespace circularDependency
{
    namespace DependencyTypeUtils
    {
        std::string toString(DependencyType type)
        {
            switch (type)
            {
            case DependencyType::GENERIC_SUBSTITUTION:
                return "generic substitution";
            case DependencyType::IMPORT_CHAIN:
                return "import chain";
            case DependencyType::INTERFACE_INHERITANCE:
                return "interface inheritance";
            case DependencyType::CLASS_INHERITANCE:
                return "class inheritance";
            case DependencyType::METHOD_OVERLOAD:
                return "method overload";
            default:
                return "unknown dependency";
            }
        }
    }
}
