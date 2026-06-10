#include "SwitchContextManager.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::compiler::control
{
    void SwitchContextManager::enterSwitch(size_t entryOffset)
    {
        SwitchContext ctx;
        ctx.entryOffset = entryOffset;
        switchStack.push_back(ctx);
    }

    void SwitchContextManager::exitSwitch()
    {
        if (switchStack.empty()) {
            throw errors::RuntimeException("Switch stack underflow");
        }
        switchStack.pop_back();
    }

    SwitchContextManager::SwitchContext& SwitchContextManager::currentSwitch()
    {
        if (switchStack.empty()) {
            throw errors::RuntimeException("Not in a switch context");
        }
        return switchStack.back();
    }

    bool SwitchContextManager::isInSwitch() const
    {
        return !switchStack.empty();
    }

    void SwitchContextManager::registerBreak(size_t jumpOffset)
    {
        if (switchStack.empty()) {
            throw errors::RuntimeException("Break outside of switch");
        }
        switchStack.back().breakJumps.push_back(jumpOffset);
    }

    const std::vector<size_t>& SwitchContextManager::getBreakJumps() const
    {
        if (switchStack.empty()) {
            throw errors::RuntimeException("Not in a switch context");
        }
        return switchStack.back().breakJumps;
    }

    size_t SwitchContextManager::getCurrentSwitchEntryOffset() const
    {
        if (switchStack.empty()) {
            throw errors::RuntimeException("Not in a switch context");
        }
        return switchStack.back().entryOffset;
    }
}
