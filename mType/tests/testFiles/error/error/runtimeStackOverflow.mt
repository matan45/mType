// Infinite recursion should trip the VM's call-stack overflow guard
// (DEFAULT_MAX_CALL_STACK_SIZE = 1000). The recursive call is wrapped in
// arithmetic so the JIT cannot tail-optimize it into a loop — every frame
// must actually be pushed on the call stack.
// VirtualMachine::pushCallFrame throws a RuntimeException whose message
// starts with "Stack overflow:".

function recurse(int depth): int {
    return recurse(depth + 1) + 1;
}

int result = recurse(0);
print(result);
