// Test that async functions execute in correct order
// Validates execution flow and ordering guarantees

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Execution Order Test ===");

class OrderTracker {
    int step;

    public constructor(int s) {
        this.step = s;
    }

    public function getStep(): int {
        return this.step;
    }
}

function async step1(): Promise<OrderTracker> {
    print("Executing step 1");
    OrderTracker t = new OrderTracker(1);
    return t;
}

function async step2(): Promise<OrderTracker> {
    print("Executing step 2");
    OrderTracker t = new OrderTracker(2);
    return t;
}

function async step3(): Promise<OrderTracker> {
    print("Executing step 3");
    OrderTracker t = new OrderTracker(3);
    return t;
}

function async testExecutionOrder(): Promise<Int> {
    print("Starting ordered execution");

    // Sequential awaits should maintain order
    OrderTracker t1 = await step1();
    print("Completed step: " + t1.getStep());

    OrderTracker t2 = await step2();
    print("Completed step: " + t2.getStep());

    OrderTracker t3 = await step3();
    print("Completed step: " + t3.getStep());

    int total = t1.getStep() + t2.getStep() + t3.getStep();
    print("Total steps: " + total);

    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await testExecutionOrder();
    print("All steps completed: " + result);
    return result;
}

main();
