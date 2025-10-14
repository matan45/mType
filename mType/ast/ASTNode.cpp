#include "ASTNode.hpp"
#include <stdexcept>

namespace ast {

    std::unique_ptr<ASTNode> ASTNode::clone() const {
        // Default implementation throws - subclasses must override
        throw std::runtime_error("clone() not implemented for this AST node type");
    }

}
