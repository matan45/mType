// Test: finally block runs after a catch handles an async-thrown exception
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

function async bad(): Promise<Int> {
    throw new Exception("boom");
    return new Int(0);
}

function async runTest(): Promise<Int> {
    Int result = new Int(-1);
    try {
        result = await bad();
        print("unreachable");
    } catch (Exception e) {
        print("caught");
    } finally {
        print("finally");
    }
    return result;
}

function async main(): Promise<void> {
    Int r = await runTest();
    print("after: " + r.getValue());
}

main();
