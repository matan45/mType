// Test: Async method called from C++ via callMethod
// Verifies that async methods with await work when invoked through the interop path.
// The test framework calls onInteropTest(string) from C++, which is an async method.

import * from "../../lib/primitives/String.mt";

@Script
public class AsyncCallMethodTest {

    constructor() {
    }

    public function onStart(): void {
        print("onStart called");
    }

    public function onUpdate(float deltaTime): void {
    }

    public function onDestroy(): void {
    }

    // Async helper that returns a resolved value
    private static function async resolveValue(String value): Promise<String> {
        return value;
    }

    // Called by the C++ test framework with native std::string arguments
    // This is an async method — tests that callMethod schedules it via EventLoop
    public function async onInteropTest(string msg): Promise<void> {
        print("before await: " + msg);
        String result = await AsyncCallMethodTest::resolveValue("resolved-" + msg);
        print("after await: " + result);
    }
}
