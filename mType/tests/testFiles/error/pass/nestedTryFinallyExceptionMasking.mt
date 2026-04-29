// Outer try wraps inner try. Inner try throws Ex1, inner finally throws Ex2.
// Outer catch must observe Ex2 (Ex1 is masked / lost).
import * from "../../lib/exceptions/Exception.mt";

class Ex1 extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class Ex2 extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

function run(): void {
    try {
        try {
            print("Inner try: throwing Ex1");
            throw new Ex1("inner-original");
        } finally {
            print("Inner finally: throwing Ex2");
            throw new Ex2("inner-finally");
        }
    } catch (Ex2 e) {
        print("Outer caught Ex2: " + e.getMessage());
    } catch (Ex1 e) {
        print("Outer caught Ex1 (unexpected): " + e.getMessage());
    }
}

function main(): void {
    print("Testing nested try/finally exception masking");
    run();
    print("Nested-try-finally-masking test completed");
}

main();
