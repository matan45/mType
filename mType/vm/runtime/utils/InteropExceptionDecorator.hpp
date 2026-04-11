#pragma once
#include <vector>
#include "../context/ExecutionContext.hpp"  // CallFrame

namespace errors { class ScriptException; }
namespace vm::bytecode { class BytecodeProgram; }

namespace vm::runtime::utils
{
    /**
     * Decorates an in-flight exception caught in a VM interop entry point
     * with two pieces of information:
     *   1. A SourceLocation pulled from the innermost script call frame's
     *      call site (only if the exception's location is still the
     *      sentinel default — never clobbers a meaningful caller-set loc).
     *   2. A stack trace formatted to match ExceptionExecutor::handleThrow,
     *      one frame per vector entry, innermost-first.
     *
     * No-op when the call stack is empty (pure-C++ caller with no script
     * frames). Sister utility to ErrorLocationHelper, but consumes the
     * call stack directly because interop entry points don't have an
     * ExecutionContext or a meaningful current instructionPointer.
     *
     * MYT-46.
     */
    void decorateFromCallStack(errors::ScriptException& e,
                               const std::vector<CallFrame>& callStack,
                               const vm::bytecode::BytecodeProgram* program);
}
