#pragma once
#include <string>

namespace services
{
    class ScriptInterpreter
    {
    public:
        void runScript(const std::string& filename);
    };
}

