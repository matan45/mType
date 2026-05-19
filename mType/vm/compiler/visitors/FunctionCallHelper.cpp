#include "FunctionCallHelper.hpp"
#include <cstddef>
#include <cstdint>
#include "GenericScopeHelper.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"
#include <optional>

namespace vm::compiler::visitors
{
    FunctionCallHelper::FunctionCallHelper(CompilerContext& context)
        : ctx(context)
        , overloadResolver(std::make_unique<overload::OverloadResolutionHelper>(context))
    {
    }

    std::string FunctionCallHelper::inferTypeFromArgument(ast::ASTNode* argument)
    {
        value::ValueType argType = ctx.typeInference.inferExpressionType(argument);

        if (argType == value::ValueType::VOID)
        {
            return "";
        }

        // MYT-282: arrays bind generic type parameters to their precise full form
        // ("int[]", "Animal[]") rather than the coarse "array" string — otherwise
        // `<T extends Object> wrap(T)` called with `int[]` infers T="array" and
        // conflicts with any return-context binding to "Object".
        if (argType == value::ValueType::ARRAY)
        {
            std::string arrayName = ctx.typeInference.inferExpressionClassName(argument);
            if (!arrayName.empty())
            {
                return arrayName;
            }
            return ::types::TypeConversionUtils::getTypeDisplayName(argType);
        }

        if (argType != value::ValueType::OBJECT)
        {
            return ::types::TypeConversionUtils::getTypeDisplayName(argType);
        }

        return ctx.typeInference.inferExpressionClassName(argument);
    }

    bool FunctionCallHelper::setupGenericTypeBindings(ast::FunctionCallNode* node, const std::string& functionName)
    {
        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata)
        {
            return false;
        }

        if (funcMetadata->genericTypeParameters.empty())
        {
            if (node->hasGenericTypeArguments())
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' is not generic but generic type arguments were provided",
                    node->getLocation());
            }
            return false;
        }

        std::unordered_map<std::string, std::string> typeBindings;

        // PHASE 3 three-tier inference: explicit args > argument-shape > return-context.
        if (node->hasGenericTypeArguments())
        {
            const auto& genericTypeArgs = node->getGenericTypeArguments();
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;

            if (genericTypeArgs.size() != genericTypeParams.size())
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' expects " +
                    std::to_string(genericTypeParams.size()) + " type argument(s) but got " +
                    std::to_string(genericTypeArgs.size()),
                    node->getLocation());
            }

            for (size_t i = 0; i < genericTypeParams.size(); ++i)
            {
                typeBindings[genericTypeParams[i]] = genericTypeArgs[i];
            }
        }
        else
        {
            const auto& arguments = node->getArguments();

            if (!funcMetadata->parameterTypes.empty() && !arguments.empty())
            {
                inferFromArguments(funcMetadata, arguments, typeBindings);
            }

            inferFromReturnType(funcMetadata, typeBindings, node->getLocation());

            const auto& genericTypeParams = funcMetadata->genericTypeParameters;
            std::vector<std::string> uninferredParams;

            for (const auto& param : genericTypeParams)
            {
                if (typeBindings.find(param) == typeBindings.end())
                {
                    uninferredParams.push_back(param);
                }
            }

            if (!uninferredParams.empty())
            {
                std::string paramList;
                for (size_t i = 0; i < uninferredParams.size(); ++i)
                {
                    if (i > 0) paramList += ", ";
                    paramList += "'" + uninferredParams[i] + "'";
                }

                throw errors::TypeException(
                    "Cannot infer type parameter(s) " + paramList + " for function '" +
                    functionName + "'. " +
                    "Provide explicit type arguments like '" + functionName + "<Type>(...)'",
                    node->getLocation());
            }
        }

        ctx.pushGenericTypeBindings(typeBindings);
        return true;
    }

    value::Value FunctionCallHelper::compileFunctionCall(ast::FunctionCallNode* node)
    {
        std::string functionName = node->getFunctionName();
        const auto& arguments = node->getArguments();

        // Resolve overload FIRST for regular function calls — generic type binding
        // setup needs the mangled name to find the correct metadata.
        std::string resolvedFunctionName = functionName;
        if (functionName.find("::") == std::string::npos &&
            !(ctx.inInstanceMethod || ctx.inStaticMethod))
        {
            bool hasGenericTypeArgs = node->hasGenericTypeArguments();
            std::vector<std::string> genericTypeArgs = hasGenericTypeArgs ? node->getGenericTypeArguments() : std::vector<std::string>{};
            resolvedFunctionName = overloadResolver->resolveFunctionOverload(
                functionName, arguments, node->getLocation(), hasGenericTypeArgs, genericTypeArgs);
        }

        bool hasGenericBindings = setupGenericTypeBindings(node, resolvedFunctionName);

        // PHASE 3: cache resolved return type for generic functions while bindings are active.
        if (hasGenericBindings)
        {
            const auto* funcMetadata = ctx.program.getFunction(resolvedFunctionName);
            if (funcMetadata && !funcMetadata->genericTypeParameters.empty())
            {
                std::string returnType = funcMetadata->returnType;

                if (returnType != "int" && returnType != "float" &&
                    returnType != "string" && returnType != "bool" &&
                    returnType != "void" && returnType != "object")
                {
                    std::string resolvedReturnType = ctx.resolveGenericType(returnType);
                    ctx.resolvedFunctionCallTypes[node] = resolvedReturnType;
                }
            }
        }

        validateFunctionParameters(node, functionName, arguments);

        // MYT-228: stash bindings for THIS call so the terminal CALL emit can
        // prepend BIND_TYPE_ARGS. Save/restore the previous slot so a nested
        // generic call (when args themselves contain generic calls) doesn't
        // clobber our bindings.
        std::optional<std::unordered_map<std::string, std::string>> savedBindings = std::move(pendingBindings_);
        if (hasGenericBindings)
        {
            pendingBindings_ = ctx.getCurrentGenericTypeBindings();
        }
        else
        {
            pendingBindings_.reset();
        }

        if (functionName.find("::") != std::string::npos)
        {
            emitStaticMethodCall(node, functionName, arguments);
        }
        else if ((ctx.inInstanceMethod || ctx.inStaticMethod) && ctx.currentClassNode)
        {
            emitMethodCallInClassContext(node, functionName, arguments);
        }
        else
        {
            // Use pre-resolved name to avoid double resolution.
            emitRegularFunctionCall(node, resolvedFunctionName, arguments);
        }

        pendingBindings_ = std::move(savedBindings);

        if (hasGenericBindings)
        {
            ctx.popGenericTypeBindings();
        }

        return std::monostate{};
    }

    void FunctionCallHelper::emitBindTypeArgsIfNeeded(ast::ASTNode* node)
    {
        // MYT-228: if the current call has generic bindings, emit BIND_TYPE_ARGS
        // that stages them onto ExecutionContext::pendingTypeArgs. The next
        // pushCallFrame (triggered by the terminal CALL_*) consumes them.
        if (!pendingBindings_ || pendingBindings_->empty())
        {
            return;
        }

        const auto& bindings = *pendingBindings_;
        std::vector<uint64_t> operands;
        operands.reserve(1 + 3 * bindings.size());
        operands.push_back(static_cast<uint64_t>(bindings.size()));

        for (const auto& [paramName, value] : bindings)
        {
            const auto kind = isTypeParamInScope(ctx, value)
                ? bytecode::TypeArgValueKind::ForwardFromCaller
                : bytecode::TypeArgValueKind::Concrete;
            size_t paramNameIdx = ctx.program.getConstantPool().addString(paramName);
            size_t valueIdx = ctx.program.getConstantPool().addString(value);

            operands.push_back(static_cast<uint64_t>(paramNameIdx));
            operands.push_back(static_cast<uint64_t>(kind));
            operands.push_back(static_cast<uint64_t>(valueIdx));
        }

        ctx.emitter.emitWithLocation(bytecode::OpCode::BIND_TYPE_ARGS, operands, node);
    }
}
