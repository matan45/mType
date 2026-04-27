// try throws Ex1; finally then throws Ex2; outer catches Ex2 (Ex1 is masked / lost).
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
            throw new Ex1("original");
        } finally {
            print("Inner finally: throwing Ex2 (masks Ex1)");
            throw new Ex2("masking");
        }
    } catch (Ex2 e) {
        print("Outer caught Ex2: " + e.getMessage());
    } catch (Ex1 e) {
        print("Outer caught Ex1 (unexpected): " + e.getMessage());
    }
}

function main(): void {
    print("Testing finally exception masks try exception");
    run();
    print("Finally-masks-try-exception test completed");
}

main();
