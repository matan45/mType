#include "IdentifierEnumerator.hpp"

#include "../environment/Environment.hpp"
#include "../environment/manager/Scope.hpp"
#include "../environment/manager/ScopeManager.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../environment/registry/FunctionRegistry.hpp"

#include <algorithm>

namespace diagnostics
{
    namespace
    {
        // Insert into a sorted, deduplicated vector. Linear scan because
        // the pools are small (rarely more than a few hundred names) and
        // the call site only runs on the throw path, which is cold.
        void mergeUnique(std::vector<std::string>& dest,
                         const std::vector<std::string>& source)
        {
            for (const auto& name : source)
            {
                if (std::find(dest.begin(), dest.end(), name) == dest.end())
                {
                    dest.push_back(name);
                }
            }
        }

        std::vector<std::string> sorted(std::vector<std::string> v)
        {
            std::sort(v.begin(), v.end());
            return v;
        }
    } // namespace

    std::vector<std::string> IdentifierEnumerator::visibleVariables(
        const environment::Environment* env)
    {
        if (!env) return {};

        std::vector<std::string> names;

        // Locals + outer scopes (walks parent chain).
        if (auto scopeMgr = env->getScopeManager())
        {
            if (auto current = scopeMgr->getCurrentScope())
            {
                mergeUnique(names, current->getAllVisibleVariableNames());
            }
        }

        // Globals.
        if (auto varMgr = env->getVariableManager())
        {
            mergeUnique(names, varMgr->getAllVariableNames());
        }

        return sorted(std::move(names));
    }

    std::vector<std::string> IdentifierEnumerator::visibleFunctions(
        const environment::Environment* env)
    {
        if (!env) return {};
        if (auto fnReg = env->getFunctionRegistry())
        {
            return sorted(fnReg->getAllFunctionNames());
        }
        return {};
    }

    std::vector<std::string> IdentifierEnumerator::visibleClasses(
        const environment::Environment* env)
    {
        if (!env) return {};
        if (auto classReg = env->getClassRegistry())
        {
            // ClassRegistry uses the base storage so getAllItemNames()
            // already returns every class name (already sorted).
            return classReg->getAllItemNames();
        }
        return {};
    }

    std::vector<std::string> IdentifierEnumerator::visibleIdentifiers(
        const environment::Environment* env)
    {
        if (!env) return {};

        std::vector<std::string> pool;
        mergeUnique(pool, visibleVariables(env));
        mergeUnique(pool, visibleFunctions(env));
        mergeUnique(pool, visibleClasses(env));
        return sorted(std::move(pool));
    }
}
