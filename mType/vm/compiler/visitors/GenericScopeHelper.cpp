#include "GenericScopeHelper.hpp"

#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"

namespace vm::compiler::visitors
{
    bool isTypeParamInScope(const CompilerContext& ctx, const std::string& name)
    {
        if (ctx.currentClassNode)
        {
            for (const auto& p : ctx.currentClassNode->getGenericParameters())
            {
                if (p.name == name) return true;
            }
        }
        if (ctx.currentMethodNode)
        {
            for (const auto& p : ctx.currentMethodNode->getGenericTypeParameters())
            {
                if (p.name == name) return true;
            }
        }
        if (ctx.currentFunctionNode)
        {
            for (const auto& p : ctx.currentFunctionNode->getGenericTypeParameters())
            {
                if (p.name == name) return true;
            }
        }
        return false;
    }
}
