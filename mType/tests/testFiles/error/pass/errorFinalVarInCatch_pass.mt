// Test: a `final` local declared before try/catch is reachable inside
// catch and remains valid after the catch block exits.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    final string tag = "STAGE-A";
    try {
        throw new Exception("boom");
    } catch (Exception e) {
        print(tag + ": " + e.getMessage());
    }
    print("after: " + tag);
}
main();
