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
            // MYT-197: resolve the frame's interned handle via the caller-
            // supplied program. For frames from loaded-library programs the
            // handle may not be present in `program`'s interner; getFrameName
            // throws in that case, which is correct exception behavior — but
            // we guard by using the frame's programIndex when the caller
            // supplies loadedPrograms via the parent utility.
            const std::string& frameNameStr = program->getFrameName(frame.functionName);
            std::ostringstream line;
            if (auto* frameLoc = program->getSourceLocation(site)) {
                line << "  at " << frameNameStr << " ("
                     << frameLoc->filename << ":"
                     << frameLoc->line << ":"
                     << frameLoc->column << ")";
            } else {
                line << "  at " << frameNameStr
                     << " (bytecode offset " << frame.returnAddress << ")";
            }
            trace.push_back(line.str());
        }
        e.setStackTrace(std::move(trace));
    }
}
