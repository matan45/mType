// try throws Ex1, inner catch throws Ex2, outer catch must observe Ex2 (not Ex1).
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
            throw new Ex1("first");
        } catch (Ex1 e) {
            print("Inner catch caught: " + e.getMessage());
            print("Inner catch: throwing Ex2");
            throw new Ex2("second");
        }
    } catch (Ex2 e) {
        print("Outer catch caught Ex2: " + e.getMessage());
    } catch (Ex1 e) {
        print("Outer catch caught Ex1 (unexpected): " + e.getMessage());
    }
}

function main(): void {
    print("Testing throw inside catch replaces original exception");
    run();
    print("Throw-replaces-original test completed");
}

main();
