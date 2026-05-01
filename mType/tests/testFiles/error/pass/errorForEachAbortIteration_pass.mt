// Test: throw from inside a for-each body propagates out of the loop
// to a catch surrounding the entire for-each; remaining items are
// not iterated.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    int[] vals = new int[4];
    vals[0] = 1; vals[1] = 2; vals[2] = 3; vals[3] = 4;
    int processed = 0;
    try {
        for (int v : vals) {
            if (v == 3) {
                throw new Exception("abort at " + v);
            }
            processed++;
            print("processed " + v);
        }
        print("loop completed");
    } catch (Exception e) {
        print("caught: " + e.getMessage() + " processed=" + processed);
    }
}
main();
