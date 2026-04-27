// Combo 25: Collections (Stack<Int>) + Try/Finally + Lambda
// Tests: push 5 values, throw mid-iteration; finally pops all and prints each

import * from "../../lib/collections/Stack.mt";
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class StopProcessing extends Exception {
    public constructor(string msg) : super(msg) {}
}

interface IntConsumer {
    function accept(Int x): void;
}

function drainStack(Stack<Int> s, IntConsumer c): void {
    while (!s.empty()) {
        Int top = s.pop();
        c.accept(top);
    }
}

function main(): void {
    print("=== Combo 25: Stack + TryFinally + Lambda ===");

    Stack<Int> stack = new Stack<Int>();
    IntConsumer printer = x -> print("  popped: " + x.getValue());

    try {
        print("--- Pushing ---");
        for (int i = 1; i <= 5; i++) {
            stack.push(new Int(i));
            print("pushed " + i + " size=" + stack.size());
        }

        print("--- Throwing ---");
        if (stack.size() > 0) {
            throw new StopProcessing("interrupted at size " + stack.size());
        }
    } catch (StopProcessing e) {
        print("caught: " + e.getMessage());
    } finally {
        print("--- Cleanup ---");
        drainStack(stack, printer);
        print("final size=" + stack.size());
    }

    print("=== Combo 25 Complete ===");
}

main();
