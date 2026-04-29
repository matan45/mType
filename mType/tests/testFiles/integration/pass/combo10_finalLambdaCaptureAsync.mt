// Combo 10: Final Variables + Lambda Capture + Async + Loops

import * from "../../lib/primitives/String.mt";

interface Producer {
    function produce(): string;
}

interface AsyncTask {
    function async run(): Promise<String>;
}

function async testFinalCaptureInAsync(): Promise<void> {
    print("--- Basic final capture ---");
    final string greeting = "Hello";
    final int magic = 42;

    Producer p1 = () -> greeting + " " + magic;
    string result = p1.produce();
    print(result);

    print("--- Final in async loop ---");
    for (int i = 0; i < 3; i++) {
        final int captured = i * 10;
        Producer p = () -> "value=" + captured;
        print(p.produce());
    }

    print("--- Final array in lambda ---");
    final int[] data = [1, 2, 3, 4, 5];
    Producer arrayProd = () -> {
        int sum = 0;
        for (int i = 0; i < data.length; i++) {
            sum = sum + data[i];
        }
        return "sum=" + sum;
    };
    print(arrayProd.produce());
}

function async testNestedFinalCapture(): Promise<void> {
    print("--- Nested final capture ---");
    final string outer = "OUTER";

    Producer outerProd = () -> {
        final string inner = "INNER";
        return outer + "+" + inner;
    };
    print(outerProd.produce());

    print("--- Final across await ---");
    final int beforeAwait = 100;

    Producer preAwait = () -> "pre=" + beforeAwait;
    print(preAwait.produce());

    await asyncNoop();

    Producer postAwait = () -> "post=" + beforeAwait;
    print(postAwait.produce());
}

function async asyncNoop(): Promise<void> {
    return;
}

function async main(): Promise<void> {
    print("=== Combo 10: Final + Lambda Capture + Async ===");
    await testFinalCaptureInAsync();
    await testNestedFinalCapture();
    print("=== Combo 10 Complete ===");
}

main();
