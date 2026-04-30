// Test: classic for-loop where each iteration may throw; catch handler
// uses continue to skip the remainder of the iteration.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    for (int i = 0; i < 5; i++) {
        try {
            if (i == 2 || i == 4) {
                throw new Exception("skip " + i);
            }
            print("kept " + i);
        } catch (Exception e) {
            print(e.getMessage());
            continue;
        }
    }
    print("done");
}
main();
