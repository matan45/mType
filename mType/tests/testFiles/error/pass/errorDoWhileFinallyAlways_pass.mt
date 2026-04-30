// Test: do-while loop where finally fires on every iteration regardless
// of whether the try ran cleanly or threw. Counter assertions confirm
// finally executed exactly once per loop.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    int count = 0;
    int finallyHits = 0;
    do {
        try {
            count++;
            if (count == 2) {
                throw new Exception("at " + count);
            }
            print("normal " + count);
        } catch (Exception e) {
            print("caught " + e.getMessage());
        } finally {
            finallyHits++;
            print("finally " + finallyHits);
        }
    } while (count < 3);
    print("count=" + count + " finallyHits=" + finallyHits);
}
main();
