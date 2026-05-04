#include "ExecutionContext.hpp"
#include <cstddef>
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


    void ExecutionContext::pushCallFrame(CallFrame frame)
    {
        // Check for stack overflow
        if (callStack.size() >= maxCallStackSize)
        {
            // MYT-228: a BIND_TYPE_ARGS may have populated pendingTypeArgs
            // immediately before this push. Releasing it here on the
            // throw path prevents stale bindings from leaking into the
            // next unrelated call once the exception is caught.
            pendingTypeArgs.reset();

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

        // MYT-228: consume the type-argument scratch slot into the
        // moved-in frame BEFORE push so the frame already owns its map
        // when it lands in the vector. Hot path: pendingTypeArgs is null,
        // single branch.
        if (pendingTypeArgs)
        {
            frame.typeArgBindings.adopt(pendingTypeArgs.releasePtr());
        }

        callStack.push_back(std::move(frame));
    }
}
