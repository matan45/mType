// Pinned behavior: an async function may throw synchronously before reaching
// its first await. mType permits this and surfaces the throw via the awaiter's
// try/catch when the caller actually awaits the rejected Promise.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

function async throwsImmediately(): Promise<Int> {
    throw new Exception("synchronous failure before any await");
    return new Int(0);
}

function async main(): Promise<void> {
    print("=== Throw Before First Await ===");
    try {
        Int v = await throwsImmediately();
        print("got value: " + v.getValue());
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    }
    print("after try");
}

main();
