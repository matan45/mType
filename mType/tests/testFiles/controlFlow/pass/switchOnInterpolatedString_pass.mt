// Edge Case: Switch on string built from interpolation

function classify(string name, int score): string {
    string key = $"{name}_{score > 50}";

    string result = "";
    // Switch on dynamically built string
    switch (key) {
        case "Alice_true":
            result = "Alice passed";
            break;
        case "Alice_false":
            result = "Alice failed";
            break;
        case "Bob_true":
            result = "Bob passed";
            break;
        case "Bob_false":
            result = "Bob failed";
            break;
        default:
            result = "Unknown: " + key;
            break;
    }
    return result;
}

function main(): void {
    print("=== Edge: Switch on Interpolated String ===");

    print(classify("Alice", 80));
    print(classify("Alice", 30));
    print(classify("Bob", 60));
    print(classify("Bob", 40));
    print(classify("Charlie", 90));

    // Switch on interpolation with expressions
    print("--- Expression interpolation ---");
    for (int i = 0; i < 5; i++) {
        string category = $"cat_{i % 3}";
        switch (category) {
            case "cat_0":
                print($"{i} -> category A");
                break;
            case "cat_1":
                print($"{i} -> category B");
                break;
            case "cat_2":
                print($"{i} -> category C");
                break;
            default:
                print($"{i} -> unknown");
                break;
        }
    }

    // Empty interpolation edge case
    print("--- Empty value ---");
    string empty = "";
    string built = $"prefix_{empty}_suffix";
    switch (built) {
        case "prefix__suffix":
            print("Matched empty middle");
            break;
        default:
            print("No match");
            break;
    }

    print("=== Edge Complete ===");
}

main();
