#include "ExceptionExecutor.hpp"
#include <cstddef>
#include <sstream>
#include "../../../value/ObjectInstance.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../environment/registry/VariableDefinition.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../VirtualMachine.hpp"

namespace vm::runtime
{
    // MYT-320: marker handlers (TryBegin/TryEnd/Catch/Finally) live inline in
    // the header. Only handleThrow stays here — it pulls in heavy includes
    // and is exception-cold.

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
        if (!value::isObject(exceptionValue))
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Only classes that inherit from Exception can be thrown");
        }

        auto objInstance = value::asObject(exceptionValue);

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
                // MYT-197: resolve handle via the frame's own program.
                const std::string& frameNameStr = vm->frameName(frame);
                auto* loc = context.program->getSourceLocation(callSite);
                if (loc)
                {
                    stackTrace << "  at " << frameNameStr << " ("
                              << loc->filename << ":" << loc->line << ":"
                              << loc->column << ")\n";
                }
                else
                {
                    stackTrace << "  at " << frameNameStr
                              << " (bytecode offset " << frame.returnAddress << ")\n";
                }
            }

            // Set the stackTrace field on the exception object
            objInstance->setField("stackTrace", stackTrace.str());
        }

        // Throw UserException with proper source location
        throw errors::UserException(exceptionValue, typeName, throwLocation);
    }
}
