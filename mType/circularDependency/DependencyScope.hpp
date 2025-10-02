#pragma once
#include "CircularDependencyDetector.hpp"

namespace circularDependency
{
    class DependencyScope
    {
    private:
        CircularDependencyDetector& detector_;
        DependencyType type_;
        std::string identifier_;
        bool entered_;

    public:
        explicit DependencyScope(CircularDependencyDetector& detector, DependencyType type,
                                 const std::string& identifier, const std::string& location = "")
            : detector_(detector), type_(type), identifier_(identifier), entered_(false)
        {
            entered_ = detector_.enterDependency(type_, identifier_, location);
        }

        ~DependencyScope()
        {
            if (entered_)
            {
                detector_.exitDependency(type_, identifier_);
            }
        }

        // Non-copyable, non-movable
        DependencyScope(const DependencyScope&) = delete;
        DependencyScope& operator=(const DependencyScope&) = delete;
        DependencyScope(DependencyScope&&) = delete;
        DependencyScope& operator=(DependencyScope&&) = delete;
    };
}
