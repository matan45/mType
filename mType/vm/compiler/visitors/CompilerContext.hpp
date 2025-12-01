#pragma once
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../environment/Environment.hpp"
#include "../emission/BytecodeEmitter.hpp"
#include "../variables/VariableTracker.hpp"
#include "../variables/GlobalVariableRegistry.hpp"
#include "../variables/FunctionFrameManager.hpp"
#include "../control/LoopContextManager.hpp"
#include "../control/SwitchContextManager.hpp"
#include "../control/ExceptionContextManager.hpp"
#include "../types/TypeInferenceEngine.hpp"
#include "../types/TypeValidator.hpp"
#include "../../../types/TypeSubstitutionService.hpp"
#include "../types/ExpectedTypeContext.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/ASTVisitor.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../circularDependency/CircularDependencyDetector.hpp"
#include <memory>

namespace vm::compiler
{
    class BytecodeCompiler;  // Forward declaration
}

namespace vm::compiler::validation
{
    class CompileTimeValidator;  // Forward declaration
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
        control::ExceptionContextManager& exceptionManager;
        types::TypeInferenceEngine& typeInference;
        types::TypeValidator& typeValidator;
        ::types::TypeSubstitutionService& typeSubstitutionService;
        validation::CompileTimeValidator* compileTimeValidator = nullptr;
        std::shared_ptr<circularDependency::CircularDependencyDetector> staticFieldInitDetector;

        // Class context (for member access)
        ast::ClassNode* currentClassNode = nullptr;
        bool inInstanceMethod = false;
        bool inStaticMethod = false;
        bool inStaticFieldInitializer = false;  // Track if we're compiling a static field initializer

        // Generic type binding stack for functions and methods
        std::vector<std::unordered_map<std::string, std::string>> genericTypeBindingStack;

        // Expected type context stack for bidirectional type checking
        std::vector<types::ExpectedTypeContext> expectedTypeContextStack;

        // PHASE 3: Cache for resolved generic function call return types
        // Maps FunctionCallNode* -> resolved className (e.g., "Int" instead of "T")
        // This cache is populated during function call compilation (while bindings are active)
        // and queried later during type checking (after bindings are popped)
        std::unordered_map<const ast::ASTNode*, std::string> resolvedFunctionCallTypes;

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
            control::ExceptionContextManager& excMgr,
            types::TypeInferenceEngine& typeInf,
            types::TypeValidator& typeVal,
            ::types::TypeSubstitutionService& typeSubstService
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
            , exceptionManager(excMgr)
            , typeInference(typeInf)
            , typeValidator(typeVal)
            , typeSubstitutionService(typeSubstService)
        {
        }

        // Generic type binding management
        void pushGenericTypeBindings(const std::unordered_map<std::string, std::string>& bindings)
        {
            genericTypeBindingStack.push_back(bindings);
        }

        void popGenericTypeBindings()
        {
            if (!genericTypeBindingStack.empty())
            {
                genericTypeBindingStack.pop_back();
            }
        }

        std::unordered_map<std::string, std::string> getCurrentGenericTypeBindings() const
        {
            if (genericTypeBindingStack.empty())
            {
                return {};
            }
            return genericTypeBindingStack.back();
        }

        std::string resolveGenericType(const std::string& typeName) const
        {
            // Get the most recent generic type bindings
            if (genericTypeBindingStack.empty())
            {
                return typeName;  // No bindings available
            }

            // Use TypeSubstitutionService to handle nested generics (e.g., "TypeToken<T>" -> "TypeToken<Int>")
            return typeSubstitutionService.resolveGenericType(typeName, genericTypeBindingStack.back());
        }

        // Expected type context management for bidirectional type checking
        void pushExpectedTypeContext(const types::ExpectedTypeContext& context)
        {
            expectedTypeContextStack.push_back(context);
        }

        void popExpectedTypeContext()
        {
            if (!expectedTypeContextStack.empty())
            {
                expectedTypeContextStack.pop_back();
            }
        }

        types::ExpectedTypeContext getCurrentExpectedTypeContext() const
        {
            if (expectedTypeContextStack.empty())
            {
                return types::ExpectedTypeContext::none();
            }
            return expectedTypeContextStack.back();
        }

        bool hasExpectedTypeContext() const
        {
            return !expectedTypeContextStack.empty() &&
                   expectedTypeContextStack.back().isActive;
        }
    };
}
