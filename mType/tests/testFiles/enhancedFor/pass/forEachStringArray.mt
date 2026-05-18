// Test enhanced for-loop over class-wrapper String[] array
import * from "../../lib/primitives/String.mt";

function main(): void {
    print("Testing enhanced for-loop over String[]:");

    String[] names = new String[3];
    names[0] = new String("alpha");
    names[1] = new String("beta");
    names[2] = new String("gamma");

    int count = 0;
    for (String n : names) {
        print(n);
        count = count + 1;
    }
    print("Count: " + count);

    // Empty class-wrapper array
    String[] empty = new String[0];
    int emptyCount = 0;
    for (String s : empty) {
        emptyCount = emptyCount + 1;
    }
    print("Empty count: " + emptyCount);

    print("for-each over String[] test passed!");
}

main();
