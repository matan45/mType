#pragma once
#include "DependencyType.hpp"
#include "CircularDependencyException.hpp"
#include "DependencyTypeUtils.hpp"

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
                  "Depth limit exceeded for " + DependencyTypeUtils::toString(type),
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
            return "Depth limit exceeded: " + DependencyTypeUtils::toString(type_) +
                " depth " + std::to_string(currentDepth_) +
                " exceeds maximum " + std::to_string(maxDepth_) +
                (!location_.empty() ? " at " + location_ : "");
        }
    };
}
