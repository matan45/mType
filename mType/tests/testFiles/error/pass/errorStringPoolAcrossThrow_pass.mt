// Test: identical string literals declared in try and catch resolve
// to the same pooled reference; reference equality (==) holds across
// the throw boundary.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    string before = "POOLED";
    try {
        throw new Exception("flow");
    } catch (Exception e) {
        string after = "POOLED";
        print(before == after);
        print(e.getMessage());
    }
    string later = "POOLED";
    print(before == later);
}
main();
