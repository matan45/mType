// Combo 32: Switch on a boxed Int / String unboxed via getValue()
// Tests: feeding unboxed primitive into a switch with multiple cases + default

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function classifyInt(Int boxed): string {
    string label = "";
    switch (boxed.getValue()) {
        case 0:
            label = "zero";
            break;
        case 1:
            label = "one";
            break;
        case 2:
            label = "two";
            break;
        case 10:
            label = "ten";
            break;
        default:
            label = "other";
            break;
    }
    return label;
}

function classifyString(String boxed): string {
    string label = "";
    switch (boxed.getValue()) {
        case "red":
            label = "warm";
            break;
        case "orange":
            label = "warm";
            break;
        case "blue":
            label = "cool";
            break;
        case "green":
            label = "cool";
            break;
        default:
            label = "neutral";
            break;
    }
    return label;
}

function main(): void {
    print("=== Combo 32: Boxed Switch + Unbox ===");

    print("--- Int switch ---");
    Int[] nums = [new Int(0), new Int(1), new Int(2), new Int(7), new Int(10), new Int(99)];
    for (int i = 0; i < nums.length; i++) {
        Int n = nums[i];
        print(n.getValue() + " -> " + classifyInt(n));
    }

    print("--- String switch ---");
    String[] colors = [new String("red"), new String("blue"), new String("green"),
                       new String("purple"), new String("orange")];
    for (int i = 0; i < colors.length; i++) {
        String c = colors[i];
        print(c.getValue() + " -> " + classifyString(c));
    }

    print("=== Combo 32 Complete ===");
}

main();
