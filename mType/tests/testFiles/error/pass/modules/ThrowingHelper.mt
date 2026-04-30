// Helper module exposing a free throwing function that the importer
// catches across the module boundary.
import * from "../../../lib/exceptions/Exception.mt";

function divideOrThrow(int a, int b): int {
    if (b == 0) {
        throw new Exception("divide by zero");
    }
    return a / b;
}
