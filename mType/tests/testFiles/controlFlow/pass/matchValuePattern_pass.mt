// Test: match statement with value patterns (int, string, bool literals)

function testIntMatch(int code): void {
    match (code) {
        case 200 -> print("OK");
        case 404 -> print("Not Found");
        case 500 -> print("Server Error");
        default -> print("Other: " + code);
    }
}

function testStringMatch(string color): void {
    match (color) {
        case "red" -> print("Stop");
        case "green" -> print("Go");
        case "yellow" -> print("Caution");
        default -> print("Unknown color");
    }
}

function testBoolMatch(bool flag): void {
    match (flag) {
        case true -> print("Enabled");
        case false -> print("Disabled");
    }
}

function main(): void {
    testIntMatch(200);
    testIntMatch(404);
    testIntMatch(500);
    testIntMatch(301);

    testStringMatch("red");
    testStringMatch("green");
    testStringMatch("blue");

    testBoolMatch(true);
    testBoolMatch(false);
}
main();
