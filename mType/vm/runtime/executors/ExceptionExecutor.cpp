#include "ExceptionExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/global/VariableDefinition.hpp"
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

        // Get exception type name and populate stack trace
        std::string typeName = "Exception";
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue))
        {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue);
            if (objInstance)
            {
                typeName = objInstance->getTypeName();

                // Generate stack trace from call stack with source location information
                std::ostringstream stackTrace;

                // Add current location (where the throw happened)
                auto* currentLoc = context.program->getSourceLocation(context.instructionPointer);
                if (currentLoc)
                {
                    stackTrace << "  at " << currentLoc->filename << ":"
                              << currentLoc->line << ":" << currentLoc->column << "\n";
                }
                else
                {
                    stackTrace << "  at <unknown location>\n";
                }

                // Add call stack frames
                for (const auto& frame : context.callStack)
                {
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
        }

        // Throw UserException - this will be caught by the VM's C++ exception handling
        throw errors::UserException(exceptionValue, typeName, errors::SourceLocation());
    }

    void ExceptionExecutor::handleFinally(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // FINALLY marks the start of a finally block
        // The finally block code follows this instruction
        // This is a no-op - the finally block just executes normally
    }
}
