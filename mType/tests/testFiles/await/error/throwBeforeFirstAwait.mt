// Error: async function throws synchronously before reaching its first await,
// and the outer code does not catch the exception. Should surface as unhandled.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

function async throwsImmediately(): Promise<Int> {
    // Throws before any await, so the rejection is produced synchronously
    throw new Exception("synchronous failure before any await");
    return new Int(0);
}

function async main(): Promise<void> {
    // No try/catch around the await - this must propagate as unhandled
    Int v = await throwsImmediately();
    print("unreachable: " + v.getValue());
}

main();
