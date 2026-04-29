// Combo 30: Enhanced for-each over ArrayList<Int> with try/catch inside the loop
// Tests: iteration continues after a thrown-and-caught error mid-loop

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class BadValueException extends Exception {
    public constructor(string msg) : super(msg) {}
}

function main(): void {
    print("=== Combo 30: ForEach + Generic + TryCatch ===");

    ArrayList<Int> values = new ArrayList<Int>();
    values.add(new Int(1));
    values.add(new Int(-2));
    values.add(new Int(3));
    values.add(new Int(0));
    values.add(new Int(5));
    values.add(new Int(-7));

    int processed = 0;
    int skipped = 0;

    for (Int v : values) {
        try {
            int x = v.getValue();
            if (x < 0) {
                throw new BadValueException("negative " + x);
            }
            if (x == 0) {
                throw new BadValueException("zero not allowed");
            }
            print("ok: " + (x * x));
            processed = processed + 1;
        } catch (BadValueException e) {
            print("skip: " + e.getMessage());
            skipped = skipped + 1;
        }
    }

    print("processed=" + processed + " skipped=" + skipped);
    print("=== Combo 30 Complete ===");
}

main();
