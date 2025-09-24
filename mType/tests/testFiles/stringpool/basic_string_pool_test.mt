// Basic StringPool functionality test
// This test verifies that the string pool correctly handles string interning

function testStringPoolBasics(): void {
    string str1 = "hello";
    string str2 = "world";
    string str3 = "hello";  // Same as str1, should reuse from pool

    print("Testing basic string functionality");
    print(str1);
    print(str2);
    print(str3);

    if (str1 == str3) {
        print("String comparison works correctly");
    }

    // Test string concatenation
    string combined = str1 + " " + str2;
    print("Combined: " + combined);
}

function testStringLiterals() : void {
    // Test with various string literals that should be interned
    print("Testing string literals");
    print("Test");
    print("Another");
    print("Test");  // Duplicate, should reuse from pool

    string longString = "This is a longer string that should still be interned by the pool";
    print(longString);
}

function testStringVariables():void  {
    // Test string variable assignments
    string name = "Alice";
    string greeting = "Hello";
    string message = greeting + " " + name;

    print(name);
    print(message);

    // Reassign variables
    name = "Bob";
    message = greeting + " " + name;
    print(name);
    print(message);
}

// Entry point
testStringPoolBasics();
testStringLiterals();
testStringVariables();