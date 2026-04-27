// Combo 17: Async + String Interpolation
// Tests: multiple awaits feeding into a single $"..." interpolation

import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

function async fetchName(): Promise<String> {
    return new String("Alice");
}

function async fetchGreeting(): Promise<String> {
    return new String("Hello");
}

function async fetchCount(): Promise<Int> {
    return new Int(7);
}

function async fetchTitle(): Promise<String> {
    return new String("Engineer");
}

function async fetchYears(int base): Promise<Int> {
    return new Int(base + 5);
}

function async greet(): Promise<String> {
    String greeting = await fetchGreeting();
    String name = await fetchName();
    string line = $"{greeting.getValue()}, {name.getValue()}!";
    return new String(line);
}

function async profile(): Promise<String> {
    String name = await fetchName();
    String title = await fetchTitle();
    Int count = await fetchCount();
    string line = $"{name.getValue()} the {title.getValue()} has {count.getValue()} tasks";
    return new String(line);
}

function async tenure(): Promise<String> {
    String name = await fetchName();
    Int years = await fetchYears(10);
    string line = $"{name.getValue()} has {years.getValue()} years experience";
    return new String(line);
}

function async main(): Promise<void> {
    print("=== Combo 17: Async + String Interpolation ===");

    print("--- Greet ---");
    String g = await greet();
    print(g.getValue());

    print("--- Profile ---");
    String p = await profile();
    print(p.getValue());

    print("--- Tenure ---");
    String t = await tenure();
    print(t.getValue());

    print("--- Inline composite ---");
    String name = await fetchName();
    Int count = await fetchCount();
    String title = await fetchTitle();
    string composite = $"{name.getValue()}/{count.getValue()}/{title.getValue()}";
    print(composite);

    print("=== Combo 17 Complete ===");
}

main();
