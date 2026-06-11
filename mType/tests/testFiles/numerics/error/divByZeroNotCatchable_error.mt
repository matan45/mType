// Error: pins BY-DESIGN behavior — VM runtime errors (MT-E5005 family) are a
// separate channel from script-thrown Exceptions and are NOT catchable by a
// script-level try/catch. The catch below must not intercept the error.
import * from "../../lib/exceptions/Exception.mt";

function divide(int a, int b): int {
    return a / b;
}
function main(): void {
    try {
        print(divide(10, 0));
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    }
}
main();
