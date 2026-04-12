// Regression test: MYT-63 — exception catch dispatch in standalone exe
// Bug: populateClassFromMetadata() did not link parent classes from the
//      environment ClassRegistry, only from bytecode metadata. This caused
//      isInstanceOf() to fail and catch blocks to never match.
import * from "exceptions/Exception.mt";

class TestException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Test 1: Catch exact thrown type
        try {
            throw new TestException("exact match");
        } catch (TestException e) {
            print("PASS: caught TestException - " + e.getMessage());
        }

        // Test 2: Catch via base Exception class (requires parent link)
        try {
            throw new TestException("base match");
        } catch (Exception e) {
            print("PASS: caught as Exception - " + e.getMessage());
        }

        // Test 3: Catch base Exception directly
        try {
            throw new Exception("direct base");
        } catch (Exception e) {
            print("PASS: caught Exception - " + e.getMessage());
        }

        print("Regression catch test passed");
    }
}
