// Pins multi-invocation of an async lambda passed as an interface
// callback. The HOF awaits the callback twice in a row with
// different arguments; both invocations must produce distinct
// pending promises and settle in source order.

import { Int } from "../../lib/primitives/Int.mt";

interface AsyncDoubler {
    function async apply(int x): Promise<Int>;
}

print("=== Async Lambda Passed To Hof ===");

function async invokeTwice(AsyncDoubler cb, int a, int b): Promise<Int> {
    Int first = await cb.apply(a);
    print("first: " + first.getValue());
    Int second = await cb.apply(b);
    print("second: " + second.getValue());
    return new Int(first.getValue() + second.getValue());
}

function async main(): Promise<Int> {
    AsyncDoubler dbl = async x -> {
        await delay(1);
        return new Int(x * 2);
    };

    Int total = await invokeTwice(dbl, 3, 7);
    print("total=" + total.getValue());
    print("OK");
    return total;
}

main();
