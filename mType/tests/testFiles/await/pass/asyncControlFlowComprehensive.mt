import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/exceptions/Exception.mt";
// Test async with all control flow types

function async processWithLoop(int count): Promise<String> {
    string result = "";

    for (int i = 0; i < count; i = i + 1) {
        if (i == 3) {
            continue;
        }
        if (i == 7) {
            break;
        }
        result = result + i;
    }

    return new String(result);
}

function async processWithTryCatch(int value): Promise<String> {
    try {
        if (value < 0) {
            throw new Exception("Negative value");
        }
        return new String("Success: " + value);
    } catch (Exception e) {
        return new String("Error caught");
    } finally {
        print("Finally executed");
    }
}

function async processWithSwitch(int value): Promise<String> {
    string result = "";

    switch (value) {
        case 1:
            result = "One";
            break;
        case 2:
            result = "Two";
            break;
        default:
            result = "Other";
            break;
    }

    return new String(result);
}

function async main(): Promise<void> {
    print("Testing async control flow");

    // Test loop
    String p1 = await processWithLoop(10);
    print("Loop result: " + p1.toString());

    // Test try-catch
    String p2 = await processWithTryCatch(5);
    print("Try-catch result: " + p2.toString());

    String p3 = await processWithTryCatch(-5);
    print("Try-catch error: " + p3.toString());

    // Test switch
    String p4 = await processWithSwitch(1);
    print("Switch result: " + p4.toString());

    print("Async control flow test completed");
}

main();
