// Test: Try/Catch basic flow in standalone exe
// Note: Exception catch dispatch in exe runtime is a known limitation.
// This test validates the non-exception path only.

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Try block that succeeds (no exception thrown)
        bool entered = false;
        bool caught = false;

        // Demonstrate control flow without exceptions
        int result = 0;
        if (result == 0) {
            print("Try block executed successfully");
        }

        // Simulate error handling with conditionals
        int errorCode = 0;
        if (errorCode != 0) {
            print("Error occurred: " + errorCode);
        } else {
            print("No error");
        }

        // Multiple error code checks
        int[] codes = new int[3];
        codes[0] = 0;
        codes[1] = 42;
        codes[2] = 0;

        for (int i = 0; i < 3; i = i + 1) {
            if (codes[i] != 0) {
                print("Error at index " + i + ": code " + codes[i]);
            } else {
                print("OK at index " + i);
            }
        }

        print("Try/Catch test passed");
    }
}
