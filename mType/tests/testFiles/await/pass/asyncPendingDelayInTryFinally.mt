// Pins try-block suspend across delay(1) followed by deterministic
// finally execution after resume. Exercises the interaction between
// the suspend/resume bookkeeping and the try/finally bytecode region:
// finally must run exactly once after the resumed task returns
// through the try-block end.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Delay In Try Finally ===");

function async runWithFinally(): Promise<Int> {
    try {
        print("try: before await");
        await delay(1);
        print("try: after await");
        return new Int(99);
    } finally {
        print("finally: ran");
    }
}

function async main(): Promise<Int> {
    Int v = await runWithFinally();
    print("main: value=" + v.getValue());
    print("OK");
    return v;
}

main();
