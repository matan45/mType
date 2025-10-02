#pragma once
#include "DependencyType.hpp"
#include "CircularDependencyException.hpp"

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
                  "True circular dependency detected in " + dependencyTypeToString(type),
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
                dependencyTypeToString(type_) +
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
