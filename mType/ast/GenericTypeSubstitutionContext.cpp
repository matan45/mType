#include "GenericTypeSubstitutionContext.hpp"
#include <cstddef>

namespace ast
{
    GenericTypeSubstitutionContext::GenericTypeSubstitutionContext()
        : detector(std::make_shared<circularDependency::CircularDependencyDetector>())
    {
    }

    GenericTypeSubstitutionContext::GenericTypeSubstitutionContext(const circularDependency::CircularDependencyConfig& config)
        : detector(std::make_shared<circularDependency::CircularDependencyDetector>(config))
    {
    }

    bool GenericTypeSubstitutionContext::enterSubstitution(const std::string& paramName, const std::string& location)
    {
        // Use enhanced circular dependency detection
        std::string fullLocation = currentLocation;
        if (!location.empty()) {
            fullLocation += " at " + location;
        }

        return detector->enterDependency(
            circularDependency::DependencyType::GENERIC_SUBSTITUTION,
            paramName,
            fullLocation
        );
    }

    void GenericTypeSubstitutionContext::exitSubstitution(const std::string& paramName)
    {
        detector->exitDependency(
            circularDependency::DependencyType::GENERIC_SUBSTITUTION,
            paramName
        );
    }

    std::vector<std::string> GenericTypeSubstitutionContext::getCurrentChain() const
    {
        return detector->getDependencyChain(circularDependency::DependencyType::GENERIC_SUBSTITUTION);
    }

    int GenericTypeSubstitutionContext::getCurrentDepth() const
    {
        return detector->getCurrentDepth(circularDependency::DependencyType::GENERIC_SUBSTITUTION);
    }

    std::string GenericTypeSubstitutionContext::getChainString() const
    {
        auto chain = getCurrentChain();
        if (chain.empty()) {
            return "(empty)";
        }

        std::string result = chain[0];
        for (size_t i = 1; i < chain.size(); ++i) {
            result += " -> " + chain[i];
        }
        return result;
    }
}
