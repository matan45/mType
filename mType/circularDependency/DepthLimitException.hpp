#pragma once
#include "DependencyType.hpp"
#include "CircularDependencyException.hpp"

namespace circularDependency
{
    class DepthLimitException : public CircularDependencyException
    {
    private:
        DependencyType type_;
        int currentDepth_;
        int maxDepth_;

    public:
        DepthLimitException(DependencyType type, int current, int max,
                                     const std::vector<std::string>& chain,
                                     const std::string& location = "")
            : CircularDependencyException(
                  "Depth limit exceeded for " + dependencyTypeToString(type),
                  chain, location)
              , type_(type), currentDepth_(current), maxDepth_(max)
        {
        }

        ~DepthLimitException() override = default;

        DependencyType getType() const { return type_; }
        int getCurrentDepth() const { return currentDepth_; }
        int getMaxDepth() const { return maxDepth_; }

        std::string getDetailedMessage() const override
        {
            return "Depth limit exceeded: " + dependencyTypeToString(type_) +
                " depth " + std::to_string(currentDepth_) +
                " exceeds maximum " + std::to_string(maxDepth_) +
                (!location_.empty() ? " at " + location_ : "");
        }

    private:
        static std::string dependencyTypeToString(DependencyType type)
        {
            switch (type)
            {
            case DependencyType::GENERIC_SUBSTITUTION: return "generic substitution";
            case DependencyType::IMPORT_CHAIN: return "import chain";
            case DependencyType::INTERFACE_INHERITANCE: return "interface inheritance";
            case DependencyType::CLASS_INHERITANCE: return "class inheritance";
            case DependencyType::METHOD_OVERLOAD: return "method overload";
            default: return "unknown dependency";
            }
        }
    };
}
