#include "GenericTypeParameter.hpp"
#include <sstream>

namespace ast
{
    std::string GenericTypeParameter::toString() const
    {
        std::ostringstream oss;
        oss << name;

        if (hasConstraints())
        {
            oss << " extends ";
            for (size_t i = 0; i < constraints.size(); ++i)
            {
                if (i > 0) oss << ", ";
                oss << constraints[i];
            }
        }

        return oss.str();
    }
}
