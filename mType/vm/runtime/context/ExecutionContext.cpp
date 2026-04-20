#include "ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include <cassert>
#include <sstream>

namespace vm::runtime
{
    // MYT-134: release every borrowed ObjectInstance* back to the pool in reverse
    // order. Called from the destructor — must be noexcept so exception unwind
    // does not call std::terminate.
    void StackFrameObjects::releaseAll() noexcept
    {
        auto& pool = value::ObjectInstancePool::getInstance();
        for (auto it = slots.rbegin(); it != slots.rend(); ++it)
        {
            pool.releaseRaw(*it);
        }
        slots.clear();
    }

    StackFrameObjects::~StackFrameObjects()
    {
        if (!slots.empty()) releaseAll();
    }

    StackFrameObjects::StackFrameObjects(const StackFrameObjects& other)
    {
        // Copy-as-empty: the source must not hold live pointers at the time of
        // the copy. Call-frame lifecycle guarantees this (see struct doc).
        assert(other.slots.empty()
               && "StackFrameObjects: copy of a frame with live stack objects "
                  "would orphan or double-release pool slots");
        (void)other;
    }

    StackFrameObjects& StackFrameObjects::operator=(const StackFrameObjects& other)
    {
        if (this != &other)
        {
            assert(other.slots.empty()
                   && "StackFrameObjects: copy-assign from a frame with live "
                      "stack objects would orphan or double-release pool slots");
            if (!slots.empty()) releaseAll();
        }
        return *this;
    }

    StackFrameObjects& StackFrameObjects::operator=(StackFrameObjects&& other) noexcept
    {
        if (this != &other)
        {
            if (!slots.empty()) releaseAll();
            slots = std::move(other.slots);
            other.slots.clear();
        }
        return *this;
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
    }
}
