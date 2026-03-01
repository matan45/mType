#include "ExceptionExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/global/VariableDefinition.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include <sstream>

namespace vm::runtime
{
    ExceptionExecutor::ExceptionExecutor(ExecutionContext& ctx)
        : context(ctx)
    {
    }

    void ExceptionExecutor::handleTryBegin(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // TRY_BEGIN marks the start of a try block
        // Just mark that we're entering a try block - actual exception handling
        // is done in C++ try-catch in the VM's interpretLoop
        // This is a no-op for now - the real work happens when THROW is executed
    }

    void ExceptionExecutor::handleTryEnd(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // TRY_END is never reached during normal execution because the JUMP before it
        // skips over it and goes directly to the CATCH or FINALLY blocks.
        // This instruction only serves as a marker in the bytecode.
        // If we somehow reach here, it's a no-op.
    }

    void ExceptionExecutor::handleCatch(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // CATCH should not be executed during normal flow
        // It's only reached when an exception is thrown
        // The operand contains the exception type index from constant pool
        // For now, this is a no-op during normal execution
        // The catch block body follows this instruction
    }

    void ExceptionExecutor::handleThrow(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Pop the exception object from the stack
        value::Value exceptionValue = context.stackManager->pop();

        // Get source location where the throw happened
        auto* currentLoc = context.program->getSourceLocation(context.instructionPointer);
        errors::SourceLocation throwLocation;
        if (currentLoc)
        {
            throwLocation = errors::SourceLocation(currentLoc->filename, currentLoc->line, currentLoc->column);
        }

        // Validate that only Exception objects (or subclasses) can be thrown
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue))
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Only classes that inherit from Exception can be thrown");
        }

        auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue);

        // Verify the object inherits from Exception
        if (objInstance)
        {
            auto classDef = objInstance->getClassDefinition();
            bool inheritsFromException = false;

            // Check if this is Exception class or inherits from it
            if (classDef->getName() == "Exception")
            {
                inheritsFromException = true;
            }
            else
            {
                // Walk up the inheritance chain
                auto currentClass = classDef;
                while (currentClass && currentClass->hasParentClass())
                {
                    currentClass = currentClass->getParentClass();
                    if (currentClass && currentClass->getName() == "Exception")
                    {
                        inheritsFromException = true;
                        break;
                    }
                }
            }

            if (!inheritsFromException)
            {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Can only throw objects that inherit from Exception class, got: " + classDef->getName());
            }
        }

        // Get exception type name and populate stack trace
        std::string typeName = objInstance ? objInstance->getTypeName() : "Exception";

        if (objInstance)
        {
            // Generate stack trace from call stack with source location information
            std::ostringstream stackTrace;

            // Add current location (where the throw happened)
            if (currentLoc)
            {
                stackTrace << "  at " << currentLoc->filename << ":"
                          << currentLoc->line << ":" << currentLoc->column << "\n";
            }
            else
            {
                stackTrace << "  at <unknown location>\n";
            }

            // Add call stack frames (reverse order: innermost caller first)
            for (auto it = context.callStack.rbegin(); it != context.callStack.rend(); ++it)
            {
                const auto& frame = *it;
                // The returnAddress points to the instruction AFTER the call
                // We need to look at the CALL instruction itself (one before)
                size_t callSite = frame.returnAddress > 0 ? frame.returnAddress - 1 : frame.returnAddress;
                auto* loc = context.program->getSourceLocation(callSite);
                if (loc)
                {
                    stackTrace << "  at " << frame.functionName << " ("
                              << loc->filename << ":" << loc->line << ":"
                              << loc->column << ")\n";
                }
                else
                {
                    stackTrace << "  at " << frame.functionName
                              << " (bytecode offset " << frame.returnAddress << ")\n";
                }
            }

            // Set the stackTrace field on the exception object
            objInstance->setField("stackTrace", stackTrace.str());
        }

        // Throw UserException with proper source location
        throw errors::UserException(exceptionValue, typeName, throwLocation);
    }

    void ExceptionExecutor::handleFinally(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // FINALLY marks the start of a finally block
        // The finally block code follows this instruction
        // Mark that we're entering this finally block so exception handling
        // knows to skip it when searching for catch handlers
        // (exceptions thrown FROM inside finally should not be caught by the same finally)

        // Note: We need access to VM to set currentFinallyOffset
        // For now, this is a no-op and the tracking is done in VirtualMachine::interpretLoop
    }
}
