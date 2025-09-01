#pragma once
#include <string>

namespace runtimeTypes
{
    class Definition
    {
    private:
        std::string name;

    public:
        explicit Definition(const std::string& n = ""); 

        virtual ~Definition() = default;
        
        // Getter for the name
        const std::string& getName() const { return name; }
        
        
    };
}
