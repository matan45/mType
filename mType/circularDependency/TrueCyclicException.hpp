#pragma once
#include "DependencyType.hpp"
#include <cstddef>
#include "CircularDependencyException.hpp"
#include "DependencyTypeUtils.hpp"

namespace circularDependency
{
    class TrueCyclicException : public CircularDependencyException
    {
    private:
        DependencyType type_;
        std::string cycleStart_;

    public:
        TrueCyclicException(DependencyType type, const std::string& cycleStart,
                            const std::vector<std::string>& chain,
                            const std::string& location = "")
            : CircularDependencyException(
                  "True circular dependency detected in " + DependencyTypeUtils::toString(type),
                  chain, location)
              , type_(type), cycleStart_(cycleStart)
        {
        }

        ~TrueCyclicException() override = default;

        DependencyType getType() const { return type_; }
        const std::string& getCycleStart() const { return cycleStart_; }

        std::string getDetailedMessage() const override
        {
            std::string detailed = "Circular dependency detected: " +
                DependencyTypeUtils::toString(type_) +
                " cycle starting at '" + cycleStart_ + "'";

            if (!dependencyChain_.empty())
            {
                detailed += "\nDependency chain: ";
                for (size_t i = 0; i < dependencyChain_.size(); ++i)
                {
                    if (i > 0) detailed += " -> ";
                    detailed += dependencyChain_[i];
                }
            }

            if (!location_.empty())
            {
                detailed += "\nAt: " + location_;
            }

            return detailed;
        }
    };
}
