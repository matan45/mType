import * from "../../lib/primitives/Int.mt";

// Test async with all control flow types

async function processWithLoop(int count): Promise<string> {
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

    return new Promise<string>(result);
}

async function processWithTryCatch(int value): Promise<string> {
    try {
        if (value < 0) {
            throw new Exception("Negative value");
        }
        return new Promise<string>("Success: " + value);
    } catch (Exception e) {
        return new Promise<string>("Error caught");
    } finally {
        print("Finally executed");
    }
}

async function processWithSwitch(int value): Promise<string> {
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

    return new Promise<string>(result);
}

function main(): void {
    print("Testing async control flow");

    // Test loop
    Promise<string> p1 = processWithLoop(10);
    print("Loop result: " + await p1);

    // Test try-catch
    Promise<string> p2 = processWithTryCatch(5);
    print("Try-catch result: " + await p2);

    Promise<string> p3 = processWithTryCatch(-5);
    print("Try-catch error: " + await p3);

    // Test switch
    Promise<string> p4 = processWithSwitch(1);
    print("Switch result: " + await p4);

    print("Async control flow test completed");
}

main();
