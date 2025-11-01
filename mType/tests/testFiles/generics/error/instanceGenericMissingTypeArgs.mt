import * from "../../lib/primitives/String.mt";

// Test: Error - Missing type arguments for generic method
// Expected error: Generic method requires explicit type arguments (Phase 1)

class Container {
    public function <T> identity(T value): T {
        return value;
    }
}

function main(): void {
    Container container = new Container();

    // ERROR: Missing type arguments - should be identity<String>(...)
    String result = container.identity(new String("hello"));

    print("This should not execute");
}

main();
