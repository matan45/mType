#pragma once
#include <string>
#include <vector>
#include "../../../environment/Environment.hpp"

namespace vm::compiler::types
{
    /**
     * Returns true if `typeName` resolves to a valid type:
     * a primitive (int/float/string/bool/void), Array, object, Promise,
     * a declared generic parameter in `validGenericParams`, or an existing
     * class/interface registered in `env`. Strips nullable suffix `?`,
     * array brackets, and generic angle brackets to reach the base name.
     *
     * Extracted from FunctionCompiler / MethodCompilerHelper duplicates so
     * the two paths cannot drift.
     */
    bool isValidTypeName(const std::string& typeName,
                         const std::vector<std::string>& validGenericParams,
                         environment::Environment& env);
}
