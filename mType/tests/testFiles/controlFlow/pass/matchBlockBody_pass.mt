// Test: match statement with block bodies

function process(int value): void {
    match (value) {
        case 1 -> {
            print("Processing one");
            int doubled = value * 2;
            print("Doubled: " + doubled);
        }
        case 2 -> {
            print("Processing two");
            int tripled = value * 3;
            print("Tripled: " + tripled);
        }
        default -> {
            print("Processing other: " + value);
        }
    }
}

function main(): void {
    process(1);
    process(2);
    process(5);
}
main();
