// Test exception after some awaits have completed successfully

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Partial Execution Exception Test ===");

class PartialException extends Exception {
    int failedStep;
    public constructor(int step): super() { this.failedStep = step; }
    public function getStep(): int { return this.failedStep; }
}

class StepResult {
    int stepNum;
    int value;

    public constructor(int step, int val) {
        this.stepNum = step;
        this.value = val;
    }

    public function getStepNum(): int { return this.stepNum; }
    public function getValue(): int { return this.value; }
}

function async executeStep(int stepNum): Promise<StepResult> {
    print("Executing step " + stepNum);

    if (stepNum == 3) {
        throw new PartialException(stepNum);
    }

    StepResult result = new StepResult(stepNum, stepNum * 10);
    return result;
}

function async runMultipleSteps(): Promise<Int> {
    print("Starting multiple steps");

    int total = 0;
    int completedSteps = 0;

    try {
        StepResult r1 = await executeStep(1);
        print("Step 1 complete: " + r1.getValue());
        total = total + r1.getValue();
        completedSteps = completedSteps + 1;

        StepResult r2 = await executeStep(2);
        print("Step 2 complete: " + r2.getValue());
        total = total + r2.getValue();
        completedSteps = completedSteps + 1;

        StepResult r3 = await executeStep(3);
        print("Step 3 complete: " + r3.getValue());
        total = total + r3.getValue();
        completedSteps = completedSteps + 1;

        print("This should not print");
    } catch (PartialException e) {
        print("Failed at step: " + e.getStep());
        print("Completed steps: " + completedSteps);
        print("Total so far: " + total);
    }

    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await runMultipleSteps();
    print("Final total: " + result);
    return result;
}

main();
