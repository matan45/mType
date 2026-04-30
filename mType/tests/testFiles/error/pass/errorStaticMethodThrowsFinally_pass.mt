// Test: a static method throws on bad input. The caller wraps in a
// try/catch/finally that decrements a static counter regardless,
// keeping the active-work tally consistent.
import * from "../../lib/exceptions/Exception.mt";

class Counter {
    public static int active = 0;

    public static function startWork(int n): void {
        active = active + 1;
        if (n < 0) {
            throw new Exception("bad n=" + n);
        }
    }
}

function doWork(int n): void {
    try {
        Counter::startWork(n);
        print("did " + n);
    } catch (Exception e) {
        print("err: " + e.getMessage());
    } finally {
        Counter::active = Counter::active - 1;
    }
}

function main(): void {
    doWork(5);
    doWork(-3);
    doWork(2);
    print("active=" + Counter::active);
}
main();
