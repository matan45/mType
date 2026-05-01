// Test: while-loop retry pattern. attempt() throws on tries 0-2 and
// succeeds on try 3; catch advances the counter and the loop exits
// once a non-negative result is set.
import * from "../../lib/exceptions/Exception.mt";

function attempt(int tryNum): int {
    if (tryNum < 3) {
        throw new Exception("try " + tryNum + " failed");
    }
    return tryNum * 10;
}

function main(): void {
    int tries = 0;
    int result = -1;
    while (result < 0) {
        try {
            result = attempt(tries);
            print("succeeded on try " + tries + " result " + result);
        } catch (Exception e) {
            print("retrying: " + e.getMessage());
            tries++;
        }
    }
    print("done after " + (tries + 1) + " tries");
}
main();
