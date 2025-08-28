#include "QualifiedNameNode.hpp"
#include <sstream>

namespace ast::nodes::namespaces
{
    std::string QualifiedNameNode::getFullName() const
    {
        if (qualifiers.empty()) {
            return "";
        }
        
        std::ostringstream oss;
        for (size_t i = 0; i < qualifiers.size(); ++i) {
            if (i > 0) {
                oss << "::";
            }
            oss << qualifiers[i];
        }
        
        return oss.str();
    }
}
