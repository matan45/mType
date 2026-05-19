#include "InlineCacheExecutor.hpp"

namespace vm::runtime
{
    InlineCacheExecutor::InlineCacheExecutor(ExecutionContext& ctx,
                                              VirtualMachine* vmPtr,
                                              vm::jit::ic::InlineCacheTable& table)
        : context(ctx)
        , vm(vmPtr)
        , icTable(table)
    {}

    // Method bodies live in InlineCacheExecutor_{FieldIC,MethodIC,PolyIC,FusedIC}.cpp.
}
