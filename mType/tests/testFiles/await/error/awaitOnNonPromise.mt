// Error: await applied to a non-Promise expression should be rejected.
// `await` on a plain int has no promise to wait for; the compiler /
// semantic checker must surface the type mismatch instead of silently
// passing the value through.

import { Int } from "../../lib/primitives/Int.mt";

function async main(): Promise<Int> {
    int notAPromise = 5;
    // ERROR: cannot await a non-Promise value
    int x = await notAPromise;
    return new Int(x);
}

main();
print("This should fail");
