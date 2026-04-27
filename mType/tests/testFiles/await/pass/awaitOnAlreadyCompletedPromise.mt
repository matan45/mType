// Test: awaiting an already-completed Promise yields its value with no scheduler stall
import * from "../../lib/primitives/Int.mt";

function async makeResolved(): Promise<Int> {
    return new Int(7);
}

function async runTest(): Promise<Int> {
    Promise<Int> p = makeResolved();
    print("Promise created");
    Int v = await p;
    print("Awaited value: " + v.getValue());
    return v;
}

function async main(): Promise<void> {
    Int result = await runTest();
    print("Final: " + result.getValue());
    print("Done");
}

main();
