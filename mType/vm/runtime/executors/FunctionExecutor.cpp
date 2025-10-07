#include "FunctionExecutor.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include <algorithm>

namespace vm::runtime
{
    FunctionExecutor::FunctionExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void FunctionExecutor::handleCall(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get function name from constant pool
        std::string functionName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = context.stackManager->size() - argCount;

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        // Try to find native function first
        auto nativeRegistry = context.environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName)) {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc) {
                value::Value result = nativeFunc(args);
                context.stackManager->push(result);
                return;
            }
        }

        // Try to find user-defined function in bytecode
        auto funcMetadata = context.program->getFunction(functionName);
        if (funcMetadata) {
            // Create call frame
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;  // Use the frameBase calculated before popping args
            frame.localBase = context.stackManager->size();  // Locals start after arguments (which are now popped)
            frame.functionName = functionName;
            frame.thisInstance = nullptr;

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            // Push arguments onto operand stack as locals (they will be at frameBase + slot)
            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            // Jump to function start
            context.instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
            return;
        }

        throw errors::RuntimeException("Function not found: " + functionName);
    }

    void FunctionExecutor::handleCallNative(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement native function calls
        throw errors::RuntimeException("CALL_NATIVE not yet implemented");
    }

    void FunctionExecutor::handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get static method name from constant pool (should be fully qualified: ClassName::methodName)
        std::string qualifiedName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Parse qualified name: ClassName::methodName
        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            throw errors::RuntimeException("Static method call requires qualified name (ClassName::methodName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string methodName = qualifiedName.substr(colonPos + 2);

        // Get class definition
        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + className);
        }

        // Find static method in class
        auto method = classDef->findMethod(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Static method not found: " + qualifiedName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        if (!method->isStatic()) {
            throw errors::RuntimeException("Method '" + methodName + "' is not static");
        }

        // Check access modifiers
        validateStaticMethodAccess(className, methodName, method->getAccessModifier());

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = context.stackManager->size() - argCount;

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        // Look up static method bytecode
        auto funcMetadata = context.program->getFunction(qualifiedName);
        if (funcMetadata) {
            // Create call frame for static method
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;  // Use the frameBase calculated before popping args
            frame.localBase = context.stackManager->size();  // Locals start after arguments (which are now popped)
            frame.functionName = qualifiedName;
            frame.thisInstance = nullptr;  // No 'this' for static methods

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            // Push arguments onto stack as locals (slot 0, 1, 2, ...)
            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            // Reserve space for local variables (beyond parameters)
            // localCount includes parameters, so we need (localCount - argCount) additional slots
            if (funcMetadata->localCount > argCount) {
                size_t additionalLocals = funcMetadata->localCount - argCount;
                for (size_t i = 0; i < additionalLocals; ++i) {
                    context.stackManager->push(std::monostate{});  // Initialize with null/undefined
                }
            }

            // Jump to static method start
            context.instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
        } else {
            throw errors::RuntimeException("Static method '" + qualifiedName + "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }
    }

    void FunctionExecutor::validateStaticMethodAccess(
        const std::string& className,
        const std::string& methodName,
        ast::AccessModifier accessMod
    ) {
        if (accessMod == ast::AccessModifier::PUBLIC) {
            return;  // Public methods are always accessible
        }

        // Get current execution context (the class we're executing from)
        std::string currentClassName;
        if (!context.callStack.empty()) {
            if (context.callStack.back().thisInstance) {
                // Instance method context
                currentClassName = context.callStack.back().thisInstance->getClassDefinition()->getName();
            } else {
                // Static method context - extract class name from function name (ClassName::methodName)
                const std::string& funcName = context.callStack.back().functionName;
                size_t colonPos = funcName.find("::");
                if (colonPos != std::string::npos) {
                    currentClassName = funcName.substr(0, colonPos);
                }
            }
        }

        if (accessMod == ast::AccessModifier::PRIVATE) {
            // PRIVATE: Only accessible from same class
            if (currentClassName != className) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    methodName,
                    "method",
                    ast::AccessModifier::PRIVATE,
                    className,
                    callingFrom,
                    errors::SourceLocation()
                );
            }
        } else if (accessMod == ast::AccessModifier::PROTECTED) {
            // PROTECTED: Accessible from same class and subclasses
            if (currentClassName != className) {
                // Check if current class is a subclass of target class
                bool isSubclass = false;
                if (!currentClassName.empty()) {
                    auto currentClass = context.environment->getClassRegistry()->findClass(currentClassName);
                    while (currentClass && currentClass->hasParentClass()) {
                        if (currentClass->getParentClassName() == className) {
                            isSubclass = true;
                            break;
                        }
                        // Move to parent class
                        auto parentClass = context.environment->getClassRegistry()->findClass(currentClass->getParentClassName());
                        currentClass = parentClass;
                    }
                }

                if (!isSubclass) {
                    std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                    throw errors::AccessViolationException(
                        methodName,
                        "method",
                        ast::AccessModifier::PROTECTED,
                        className,
                        callingFrom,
                        errors::SourceLocation()
                    );
                }
            }
        }
    }
}
