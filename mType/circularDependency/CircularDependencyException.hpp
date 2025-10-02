#pragma once

#include <string>
#include <vector>
#include <exception>

namespace circularDependency
{
    class CircularDependencyException : public std::exception
    {
    protected:
        std::vector<std::string> dependencyChain_;
        std::string location_;
        std::string message_;

    public:
        explicit CircularDependencyException(const std::string& message,
                                             const std::vector<std::string>& chain = {},
                                             const std::string& location = "")
            : dependencyChain_(chain)
              , location_(location)
              , message_(message)
        {
        }

        virtual ~CircularDependencyException() = default;

        // Override what() to return our custom message
        const char* what() const noexcept override
        {
            return message_.c_str();
        }

        const std::vector<std::string>& getDependencyChain() const { return dependencyChain_; }
        const std::string& getLocation() const { return location_; }

        virtual std::string getDetailedMessage() const
        {
            std::string detailed = message_;
            if (!location_.empty())
            {
                detailed += " at " + location_;
            }
            return detailed;
        }
    };
}
