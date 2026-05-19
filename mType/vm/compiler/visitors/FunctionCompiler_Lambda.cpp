#include "FunctionCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../analysis/NestedReferenceCollector.hpp"

namespace vm::compiler::visitors
{
    std::vector<variables::VariableTracker::LocalVariable> FunctionCompiler::captureScopeVariables()
    {
        std::vector<variables::VariableTracker::LocalVariable> capturedVars;
        const auto& currentLocals = ctx.variableTracker.getLocals();

        // Capture all variables from the current function frame.
        size_t currentFrameStart = ctx.functionFrameManager.isInFunction()
                                       ? ctx.functionFrameManager.currentFrame().localStartSlot
                                       : 0;

        for (const auto& local : currentLocals)
        {
            if (local.slot >= currentFrameStart)
            {
                capturedVars.push_back(local);
                // Mark captured to prevent slot reuse.
                ctx.variableTracker.markVariableAsCaptured(local.slot);
            }
        }

        return capturedVars;
    }

    std::vector<std::string> FunctionCompiler::setupLambdaFrame(ast::LambdaNode* node,
                                            const std::vector<variables::VariableTracker::LocalVariable>& capturedVars,
                                            const std::string& lambdaFuncName)
    {
        const auto& params = node->getParameters();
        std::vector<std::string> parameterTypeNames;

        // C# semantics: lambda parameters cannot shadow outer-scope variables.
        for (const auto& param : params)
        {
            for (const auto& captured : capturedVars)
            {
                if (param.name == captured.name)
                {
                    throw errors::TypeException(
                        "Lambda parameter '" + param.name + "' shadows outer scope variable. "
                        "Variable shadowing is not allowed in lambdas (C# semantics).",
                        node->getLocation());
                }
            }
        }

        ctx.functionFrameManager.enterFunctionFrame(lambdaFuncName,
                                                    "auto",
                                                    0,
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    true,
                                                    node->getIsAsync());

        // Store captured names in the frame for shadowing validation.
        auto& currentFrame = ctx.functionFrameManager.currentFrame();
        for (const auto& captured : capturedVars)
        {
            currentFrame.capturedVariableNames.push_back(captured.name);
        }

        ctx.variableTracker.beginScope();

        // Resolve lambda parameter types from the expected-type context.
        std::vector<std::pair<value::ValueType, std::string>> resolvedParamTypes;

        if (ctx.hasExpectedTypeContext())
        {
            auto expectedCtx = ctx.getCurrentExpectedTypeContext();

            if (expectedCtx.expectedType == value::ValueType::OBJECT)
            {
                std::string baseClassName = expectedCtx.getBaseClassName();

                auto interfaceDef = ctx.env->findInterface(baseClassName);

                if (interfaceDef && interfaceDef->isFunctionalInterface())
                {
                    auto* samMethod = interfaceDef->getFunctionalMethod();

                    if (samMethod && samMethod->parameters.size() == params.size())
                    {
                        std::unordered_map<std::string, std::string> bindings;

                        if (expectedCtx.hasGenericArguments())
                        {
                            auto genericArgs = expectedCtx.extractGenericArguments();
                            const auto& interfaceGenericParams = interfaceDef->getGenericParameters();

                            for (size_t i = 0; i < interfaceGenericParams.size() && i < genericArgs.size(); ++i)
                            {
                                bindings[interfaceGenericParams[i].name] = genericArgs[i];
                            }
                        }

                        for (size_t i = 0; i < samMethod->parameters.size(); ++i)
                        {
                            std::string paramTypeName = samMethod->parameters[i].second->toString();

                            if (!bindings.empty())
                            {
                                auto it = bindings.find(paramTypeName);
                                if (it != bindings.end())
                                {
                                    paramTypeName = it->second;
                                }
                            }

                            value::ValueType paramType;
                            std::string paramClassName = "";

                            if (paramTypeName == "int") paramType = value::ValueType::INT;
                            else if (paramTypeName == "float") paramType = value::ValueType::FLOAT;
                            else if (paramTypeName == "bool") paramType = value::ValueType::BOOL;
                            else if (paramTypeName == "string") paramType = value::ValueType::STRING;
                            else if (paramTypeName == "void") paramType = value::ValueType::VOID;
                            else
                            {
                                paramType = value::ValueType::OBJECT;
                                paramClassName = paramTypeName;
                            }

                            resolvedParamTypes.push_back({paramType, paramClassName});
                        }
                    }
                }
            }
        }

        // Lambda parameters occupy slots 0, 1, 2, …
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i < resolvedParamTypes.size())
            {
                ctx.variableTracker.declareLocal(params[i].name, resolvedParamTypes[i].first, resolvedParamTypes[i].second);

                if (!resolvedParamTypes[i].second.empty()) {
                    parameterTypeNames.push_back(resolvedParamTypes[i].second);
                } else {
                    std::string typeName;
                    switch (resolvedParamTypes[i].first) {
                        case value::ValueType::INT: typeName = "int"; break;
                        case value::ValueType::FLOAT: typeName = "float"; break;
                        case value::ValueType::BOOL: typeName = "bool"; break;
                        case value::ValueType::STRING: typeName = "string"; break;
                        case value::ValueType::VOID: typeName = "void"; break;
                        default: typeName = "object"; break;
                    }
                    parameterTypeNames.push_back(typeName);
                }
            }
            else
            {
                ctx.variableTracker.declareLocal(params[i].name, value::ValueType::VOID, "");
                parameterTypeNames.push_back("auto");
            }
        }

        // Captured variables occupy slots after parameters.
        for (const auto& capture : capturedVars)
        {
            ctx.variableTracker.declareLocal(capture.name, capture.type, capture.className, capture.isNullable);
        }

        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        return parameterTypeNames;
    }

    void FunctionCompiler::emitLambdaInstruction(size_t lambdaStart, ast::LambdaNode* node,
                                                 const std::vector<variables::VariableTracker::LocalVariable>&
                                                 capturedVars,
                                                 size_t currentFrameStart,
                                                 const std::vector<variables::VariableTracker::LocalVariable>&
                                                 currentLocals,
                                                 const std::string& lambdaFuncName)
    {
        const auto& params = node->getParameters();

        // Operands layout: [lambdaStart, paramCount, captureCount, parentLocalCount,
        //                   functionNameIdx, captureSlot1..N, paramNameIdx1..N,
        //                   capturedNameIdx1..N, parentNameIdx1, parentSlot1, ...]
        std::vector<uint64_t> operands;
        operands.push_back(static_cast<uint64_t>(lambdaStart));
        operands.push_back(static_cast<uint64_t>(params.size()));
        operands.push_back(static_cast<uint64_t>(capturedVars.size()));
        operands.push_back(static_cast<uint64_t>(currentLocals.size()));

        size_t funcNameIdx = ctx.program.getConstantPool().addString(lambdaFuncName);
        operands.push_back(static_cast<uint64_t>(funcNameIdx));

        for (const auto& capture : capturedVars)
        {
            size_t relativeSlot = capture.slot - currentFrameStart;
            operands.push_back(static_cast<uint64_t>(relativeSlot));
        }

        for (const auto& param : params)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(param.name);
            operands.push_back(static_cast<uint64_t>(nameIndex));
        }

        for (const auto& capture : capturedVars)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(capture.name);
            operands.push_back(static_cast<uint64_t>(nameIndex));
        }

        // Parent local name->slot mapping for late-bound access.
        for (const auto& local : currentLocals)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(local.name);
            operands.push_back(static_cast<uint64_t>(nameIndex));
            operands.push_back(static_cast<uint64_t>(local.slot));
        }

        ctx.emitter.emitWithLocation(bytecode::OpCode::LAMBDA, operands, node);
    }

    value::Value FunctionCompiler::compileLambda(ast::LambdaNode* node)
    {
        static size_t lambdaCounter = 0;
        std::string lambdaFuncName = "__lambda_" + std::to_string(lambdaCounter++);

        ctx.emitter.emitWithLocation(bytecode::OpCode::JUMP, 0u, node);
        size_t skipJump = ctx.program.getCurrentOffset() - 1;

        size_t lambdaStart = ctx.program.getCurrentOffset();

        size_t savedNextLocalSlot = ctx.variableTracker.getNextLocalSlot();

        const auto& currentLocals = ctx.variableTracker.getLocals();
        auto capturedVars = captureScopeVariables();
        size_t currentFrameStart = ctx.functionFrameManager.isInFunction()
                                       ? ctx.functionFrameManager.currentFrame().localStartSlot
                                       : 0;

        // MYT-215: a lambda created inside a loop cannot capture a variable whose
        // slot has been reassigned (the for-loop counter is the canonical case).
        // Reference capture would let every lambda in the loop see the post-loop
        // value (the closure-over-loop-var footgun). Force the user to snapshot
        // into a fresh local in the loop body. captureScopeVariables() over-
        // captures, so filter to names the lambda body actually references —
        // otherwise we'd also reject lambdas that never read the loop counter.
        if (ctx.loopManager.isInLoop())
        {
            auto refs = analysis::NestedReferenceCollector::collect(node);
            for (const auto& captured : capturedVars)
            {
                if (!captured.isMutated) continue;
                if (!refs.pessimistic && refs.names.find(captured.name) == refs.names.end()) continue;
                throw errors::TypeException(
                    "Variable '" + captured.name + "' is mutated within the enclosing "
                    "loop and cannot be captured by a lambda. Copy it to a new local "
                    "first (e.g., `int snap = " + captured.name + ";` and capture `snap`).",
                    node->getLocation());
            }
        }

        ctx.variableTracker.resetLocalSlot();

        // Pre-register lambda metadata so exception tables can be added during
        // body compilation.
        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = lambdaFuncName;
        tempMetadata.startOffset = lambdaStart;
        tempMetadata.instructionCount = 0;
        tempMetadata.localCount = 0;
        tempMetadata.parameterCount = node->getParameters().size();
        tempMetadata.returnType = "auto";
        tempMetadata.isAsync = node->getIsAsync();
        tempMetadata.isStatic = false;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(lambdaFuncName, tempMetadata);

        std::vector<std::string> parameterTypes = setupLambdaFrame(node, capturedVars, lambdaFuncName);

        // Lambda body compiles in the enclosing class context — ctx.currentClassNode
        // is already set from the enclosing class/method compilation.

        auto* body = node->getBody();
        if (node->isExpressionLambda())
        {
            body->accept(ctx.visitor);

            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }
        else
        {
            // New scope for the body so it can shadow captured variables.
            ctx.variableTracker.beginScope();

            body->accept(ctx.visitor);

            ctx.variableTracker.endScope();

            // Implicit return null if no explicit return.
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        // Capture local variable names for debugging before exiting the frame.
        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();
        size_t localCount = ctx.functionFrameManager.getLocalCount();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot >= currentFrame.localStartSlot)
            {
                size_t relativeSlot = local.slot - currentFrame.localStartSlot;
                if (relativeSlot < localCount)
                {
                    localVarNames[relativeSlot] = local.name;
                }
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t lambdaEnd = ctx.program.getCurrentOffset();

        ctx.program.patchJump(skipJump, static_cast<uint64_t>(lambdaEnd));

        // Preserve exception table built during body compilation.
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(lambdaFuncName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = lambdaFuncName;
        metadata.startOffset = lambdaStart;
        metadata.instructionCount = lambdaEnd - lambdaStart;
        metadata.localCount = localCount;
        metadata.parameterCount = node->getParameters().size();
        metadata.parameterTypes = parameterTypes;
        metadata.returnType = "auto";
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = localVarNames;

        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(lambdaFuncName, metadata);

        emitLambdaInstruction(lambdaStart, node, capturedVars, currentFrameStart, currentLocals, lambdaFuncName);

        ctx.variableTracker.setLocalSlot(savedNextLocalSlot);

        return std::monostate{};
    }
}
