// Test: enhanced-for over an int[] with item-level try/catch. Each
// "bad" item (negative) is caught and tallied; "good" items processed.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    int[] vals = new int[5];
    vals[0] = 10; vals[1] = -1; vals[2] = 20; vals[3] = -2; vals[4] = 30;
    int kept = 0;
    int skipped = 0;
    for (int v : vals) {
        try {
            if (v < 0) {
                throw new Exception("bad " + v);
            }
            kept++;
        } catch (Exception e) {
            skipped++;
        }
    }
    print("kept=" + kept + " skipped=" + skipped);
}
main();
