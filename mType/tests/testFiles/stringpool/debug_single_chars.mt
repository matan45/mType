function testSingleChars(): void {
    string str1 = "hello";
    string str2 = "world";

    print("=== Testing single character literals ===");

    // Test different single characters
    print("Space: '" + str1 + " " + str2 + "'");
    print("Underscore: '" + str1 + "_" + str2 + "'");
    print("Dash: '" + str1 + "-" + str2 + "'");
    print("Comma: '" + str1 + "," + str2 + "'");
    print("Pipe: '" + str1 + "|" + str2 + "'");
    print("Dot: '" + str1 + "." + str2 + "'");
    print("At: '" + str1 + "@" + str2 + "'");

    // Test two character literals
    print("Two chars: '" + str1 + "ab" + str2 + "'");
    print("Space+char: '" + str1 + " x" + str2 + "'");
}

testSingleChars();