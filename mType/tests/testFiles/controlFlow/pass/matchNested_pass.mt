// Test: match statement nested in loops and functions, nested match

function innerMatch(int x): string {
    string result = "";
    match (x) {
        case 1 -> result = "one";
        case 2 -> result = "two";
        default -> result = "other";
    }
    return result;
}

function main(): void {
    // Match inside a loop
    for (int i = 0; i < 4; i = i + 1) {
        match (i) {
            case 0 -> print("start");
            case 3 -> print("end");
            default -> print("middle");
        }
    }

    // Match calling function with match
    print(innerMatch(1));
    print(innerMatch(2));
    print(innerMatch(99));

    // Nested match
    int x = 1;
    int y = 10;
    match (x) {
        case 1 -> {
            match (y) {
                case 10 -> print("x=1, y=10");
                default -> print("x=1, y=other");
            }
        }
        default -> print("x=other");
    }
}
main();
