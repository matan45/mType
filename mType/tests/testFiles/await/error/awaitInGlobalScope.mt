// Error: Cannot use await in global scope

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Global Scope Error Test ===");

function async getValue(): Promise<Int> {
    return new Int(42);
}

// ERROR: Cannot await at top level / global scope
Int result = await getValue();

print("This should not execute");
