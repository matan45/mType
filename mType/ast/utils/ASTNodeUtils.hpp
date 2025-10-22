#pragma once

#include "../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::utils
{
    /**
     * @brief Clone a vector of unique_ptr<ASTNode> with null safety
     *
     * This utility function eliminates code duplication across clone() methods
     * in statement nodes that contain vectors of child nodes.
     *
     * @param nodes Vector of nodes to clone
     * @return New vector with cloned nodes
     */
    template<typename T = ASTNode>
    std::vector<std::unique_ptr<T>> cloneNodeVector(const std::vector<std::unique_ptr<T>>& nodes)
    {
        std::vector<std::unique_ptr<T>> cloned;
        cloned.reserve(nodes.size());

        for (const auto& node : nodes)
        {
            if (node)
            {
                cloned.push_back(std::unique_ptr<T>(
                    static_cast<T*>(node->clone().release())
                ));
            }
        }

        return cloned;
    }

    /**
     * @brief Safely clone a single unique_ptr<ASTNode> with null check
     *
     * This utility function provides a consistent null-safe way to clone
     * individual AST nodes.
     *
     * @param node Node to clone (may be null)
     * @return Cloned node or nullptr if input was null
     */
    inline std::unique_ptr<ASTNode> cloneNodeSafe(const std::unique_ptr<ASTNode>& node)
    {
        return node ? node->clone() : nullptr;
    }

    /**
     * @brief Safely clone a shared_ptr<ASTNode> with null check
     *
     * Used for nodes that store children as shared_ptr.
     *
     * @param node Node to clone (may be null)
     * @return Cloned node as shared_ptr or nullptr if input was null
     */
    inline std::shared_ptr<ASTNode> cloneNodeSafeShared(const std::shared_ptr<ASTNode>& node)
    {
        return node ? std::shared_ptr<ASTNode>(node->clone()) : nullptr;
    }
}
