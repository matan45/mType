// Test: reading past the end of an int[] throws a runtime exception
// that is catchable as Exception. The runtime emits "index out of bounds"
// in the message; we just verify the catch handler runs.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    int[] a = new int[3];
    a[0] = 1; a[1] = 2; a[2] = 3;
    try {
        int x = a[5];
        print("no, got " + x);
    } catch (Exception e) {
        print("oob caught");
    }
    print("len=" + a.length);
}
main();
