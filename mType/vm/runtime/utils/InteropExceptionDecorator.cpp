#include "InteropExceptionDecorator.hpp"
#include "../../../errors/ScriptException.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <sstream>

namespace vm::runtime::utils
{
    void decorateFromCallStack(errors::ScriptException& e,
                               const std::vector<CallFrame>& callStack,
                               const vm::bytecode::BytecodeProgram* program)
    {
        if (callStack.empty() || program == nullptr) {
            return;
        }

        // 1. Update the exception's SourceLocation from the innermost frame.
        //    ExceptionExecutor uses (returnAddress - 1) because returnAddress
        //    is the resume point *after* the call. Mirror that exactly.
        const auto& innermost = callStack.back();
        size_t callSite = innermost.returnAddress > 0
                              ? innermost.returnAddress - 1
                              : innermost.returnAddress;

        if (auto* loc = program->getSourceLocation(callSite)) {
            errors::SourceLocation converted(loc->filename, loc->line, loc->column);
            e.setLocation(converted);
        }

        // 2. Format every frame as one trace line. Innermost-first iteration
        //    matches ExceptionExecutor::handleThrow character-for-character so
        //    the renderer's output is consistent across exception types.
        std::vector<std::string> trace;
        trace.reserve(callStack.size());
        for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
            const auto& frame = *it;
            size_t site = frame.returnAddress > 0
                              ? frame.returnAddress - 1
                              : frame.returnAddress;
            std::ostringstream line;
            if (auto* frameLoc = program->getSourceLocation(site)) {
                line << "  at " << frame.functionName << " ("
                     << frameLoc->filename << ":"
                     << frameLoc->line << ":"
                     << frameLoc->column << ")";
            } else {
                line << "  at " << frame.functionName
                     << " (bytecode offset " << frame.returnAddress << ")";
            }
            trace.push_back(line.str());
        }
        e.setStackTrace(std::move(trace));
    }
}
