// Error: await is only legal inside async function bodies (including
// async lambdas). Using await inside a synchronous lambda body must
// be rejected. The existing awaitOutsideAsync.mt covers the regular
// function case; this covers the synchronous-lambda case.

import { Int } from "../../lib/primitives/Int.mt";

interface IntFn {
    function apply(int x): Int;
}

function async produce(): Promise<Int> {
    return new Int(1);
}

function run(): Int {
    // ERROR: synchronous lambda cannot contain await
    IntFn bad = x -> {
        Int v = await produce();
        return v;
    };
    return bad.apply(0);
}

run();
print("This should fail");
