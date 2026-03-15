// Test: match statement with default case and mixed patterns

function classify(int n): void {
    match (n) {
        case 0 -> print("zero");
        case 1 -> print("one");
        case 2 -> print("two");
        default -> print("many: " + n);
    }
}

function main(): void {
    classify(0);
    classify(1);
    classify(2);
    classify(42);
    classify(-1);
}
main();
