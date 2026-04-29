// Both try and catch bodies empty. Verify the program doesn't crash and
// continues execution past the empty try/catch.
import * from "../../lib/exceptions/Exception.mt";

function run(): void {
    print("before");
    try {
    } catch (Exception e) {
    }
    print("after");
}

function main(): void {
    print("Testing empty try and empty catch");
    run();
    print("Empty try/catch test completed");
}

main();
