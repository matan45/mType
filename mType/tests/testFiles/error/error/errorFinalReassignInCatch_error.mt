// Reassigning a `final` local from inside a catch block must be a
// compile error — final means single-assignment regardless of where
// the second assignment lives.
import * from "../../lib/exceptions/Exception.mt";

function main(): void {
    final int x = 1;
    try {
        throw new Exception("e");
    } catch (Exception e) {
        x = 2;
    }
    print(x);
}
main();
