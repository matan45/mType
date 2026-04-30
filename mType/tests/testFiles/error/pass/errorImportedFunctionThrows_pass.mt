// Test: a free function defined in an imported module throws; caller
// catches across the module boundary.
import * from "../../lib/exceptions/Exception.mt";
import * from "./modules/ThrowingHelper.mt";

function main(): void {
    try {
        int r = divideOrThrow(10, 2);
        print("ok " + r);
    } catch (Exception e) {
        print("nope");
    }
    try {
        int r = divideOrThrow(10, 0);
        print("nope " + r);
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    }
}
main();
