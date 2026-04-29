#include "ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include <sstream>

namespace vm::runtime
{

    void CallFrame::releaseStackObjects() noexcept
    {
        if (stackObjectsCount == 0) return;
        auto& pool = value::ObjectInstancePool::getInstance();
        for (size_t i = 0; i < stackObjectsCount; ++i)
        {
            if (stackObjects[i]) pool.releaseRaw(stackObjects[i]);
        }
        stackObjectsCount = 0;
    }


    void ExecutionContext::pushCallFrame(const CallFrame& frame)
    {
        // Check for stack overflow
        if (callStack.size() >= maxCallStackSize)
        {
            // Build a helpful error message with stack trace
            std::ostringstream oss;
            oss << "Stack overflow: Maximum call stack depth of "
                << maxCallStackSize << " exceeded.\n";
            oss << "This may indicate infinite recursion.\n";
            oss << "Call stack trace (most recent call first):\n";

            // Show last 10 frames to help identify recursion pattern.
            // MYT-197: resolve each frame's handle via its owning program.
            size_t startIdx = callStack.size() > 10 ? callStack.size() - 10 : 0;
            for (size_t i = startIdx; i < callStack.size(); ++i)
            {
                oss << "  [" << i << "] " << frameName(callStack[i]);
                if (!callStack[i].definingClassName.empty())
                {
                    oss << " (in class " << callStack[i].definingClassName << ")";
                }
                oss << "\n";
            }

            // Add the frame that would overflow
            oss << "  [" << callStack.size() << "] " << frameName(frame);
            if (!frame.definingClassName.empty())
            {
                oss << " (in class " << frame.definingClassName << ")";
            }
            oss << " <- stack overflow here\n";

            throw errors::RuntimeException(oss.str());
        }

        callStack.push_back(frame);

        // MYT-228: consume the type-argument scratch slot into the new
        // frame. nullopt on every non-generic call, so the hot path
        // is a single well-predicted branch.
        if (pendingTypeArgs)
        {
            callStack.back().typeArgBindings = std::move(pendingTypeArgs);
            pendingTypeArgs.reset();
        }
    }
}
