#pragma once
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../environment/Environment.hpp"
#include "../emission/BytecodeEmitter.hpp"
#include "../variables/VariableTracker.hpp"
#include "../variables/GlobalVariableRegistry.hpp"
#include "../variables/FunctionFrameManager.hpp"
#include "../control/LoopContextManager.hpp"
#include "../control/SwitchContextManager.hpp"
#include "../types/TypeInferenceEngine.hpp"
#include "../types/TypeValidator.hpp"
#include "../types/GenericTypeResolver.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/ASTVisitor.hpp"
#include "../../../value/ValueType.hpp"
#include <memory>

namespace vm::compiler
{
    class BytecodeCompiler;  // Forward declaration
}

namespace vm::compiler::visitors
{
    /**
     * Shared context for all visitor compilers
     * Provides access to all compiler components and state
     */
    struct CompilerContext
    {
        // Visitor for delegation
        ast::ASTVisitor<value::Value>& visitor;
        // Core components
        bytecode::BytecodeProgram& program;
        std::shared_ptr<environment::Environment> environment;

        // Helper components
        emission::BytecodeEmitter& emitter;
        variables::VariableTracker& variableTracker;
        variables::GlobalVariableRegistry& globalRegistry;
        variables::FunctionFrameManager& functionFrameManager;
        control::LoopContextManager& loopManager;
        control::SwitchContextManager& switchManager;
        types::TypeInferenceEngine& typeInference;
        types::TypeValidator& typeValidator;
        types::GenericTypeResolver& genericResolver;

        // Class context (for member access)
        ast::ClassNode* currentClassNode = nullptr;
        bool inInstanceMethod = false;

        CompilerContext(
            ast::ASTVisitor<value::Value>& vis,
            bytecode::BytecodeProgram& prog,
            std::shared_ptr<environment::Environment> env,
            emission::BytecodeEmitter& emit,
            variables::VariableTracker& varTracker,
            variables::GlobalVariableRegistry& globalReg,
            variables::FunctionFrameManager& funcFrameMgr,
            control::LoopContextManager& loopMgr,
            control::SwitchContextManager& switchMgr,
            types::TypeInferenceEngine& typeInf,
            types::TypeValidator& typeVal,
            types::GenericTypeResolver& genericRes
        )
            : visitor(vis)
            , program(prog)
            , environment(env)
            , emitter(emit)
            , variableTracker(varTracker)
            , globalRegistry(globalReg)
            , functionFrameManager(funcFrameMgr)
            , loopManager(loopMgr)
            , switchManager(switchMgr)
            , typeInference(typeInf)
            , typeValidator(typeVal)
            , genericResolver(genericRes)
        {
        }
    };
}
