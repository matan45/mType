// Test: an inner async function throws, an outer async awaits it and catches the exception
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

function async inner(): Promise<Int> {
    print("inner start");
    throw new Exception("inner failure");
    return new Int(0);
}

function async outer(): Promise<Int> {
    print("outer start");
    Int value = new Int(-1);
    try {
        value = await inner();
        print("unreachable after await");
    } catch (Exception e) {
        print("outer caught: " + e.getMessage());
        value = new Int(99);
    }
    return value;
}

function async main(): Promise<void> {
    Int result = await outer();
    print("result: " + result.getValue());
}

main();
