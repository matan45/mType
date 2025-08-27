#pragma once
#include <string>

namespace environment
{
    class IEnvironmentComponent
    {
    public:
        virtual ~IEnvironmentComponent() = default;
        
        virtual void initialize() = 0;
        virtual void cleanup() = 0;
        virtual std::string getComponentName() const = 0;
    };
}