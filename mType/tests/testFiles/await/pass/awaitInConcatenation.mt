// Test await in string concatenation

import { String } from "../../lib/primitives/String.mt";

print("=== Await in Concatenation Test ===");

function async getPart1(): Promise<String> {
    print("Getting part 1");
    return new String("Hello");
}

function async getPart2(): Promise<String> {
    print("Getting part 2");
    return new String(" ");
}

function async getPart3(): Promise<String> {
    print("Getting part 3");
    return new String("World");
}

function async testConcatenation(): Promise<String> {
    print("Testing string concatenation");

    // Await in concatenation
    string result = (await getPart1()).getValue() +
                    (await getPart2()).getValue() +
                    (await getPart3()).getValue();
    print("Result: " + result);

    return new String(result);
}

function async main(): Promise<String> {
    String r = await testConcatenation();
    print("Final: " + r.getValue());
    return r;
}

main();
