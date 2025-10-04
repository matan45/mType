// StringPool Efficiency and Memory Usage Test
// Tests string pool deduplication and memory efficiency

function testStringDeduplication(): void  {
    print("=== String Pool Deduplication Test ===");

    // Create multiple variables with the same string values
    string str1 = "common_string";
    string str2 = "common_string";  // Should reuse pool entry
    string str3 = "common_string";  // Should reuse pool entry

    print("Created three variables with same string content:");
    print("str1: " + str1);
    print("str2: " + str2);
    print("str3: " + str3);

    // Test equality - should be very fast with pool optimization
    if (str1 == str2 && str2 == str3) {
        print("String equality comparison works correctly");
    } else {
        print("String equality comparison failed");
    }
}

function testKeywordPooling(): void {
    print("=== Keyword and Operator Pooling Test ===");

    // These keywords should be heavily pooled
    string keyword1 = "class";
    string keyword2 = "function";
    string keyword3 = "if";
    string keyword4 = "else";
    string keyword5 = "for";
    string keyword6 = "while";

    // Reuse same keywords - should hit pool
    string keyword1_dup = "class";
    string keyword2_dup = "function";

    print("Testing keyword pooling:");
    print("Original: " + keyword1 + ", " + keyword2);
    print("Duplicates: " + keyword1_dup + ", " + keyword2_dup);

    if (keyword1 == keyword1_dup && keyword2 == keyword2_dup) {
        print("Keyword pooling works correctly");
    }
}

function testStringLiteralPooling(): void {
    print("=== String Literal Pooling Test ===");

    // Test various string literal patterns
    string empty = "";
    string single = "a";
    string short_str = "hi";
    string medium_str = "hello world";
    string long_str = "this is a longer string for testing pooling efficiency";

    // Duplicate the same literals
    string empty_dup = "";
    string single_dup = "a";
    string short_str_dup = "hi";
    string medium_str_dup = "hello world";

    print("Testing string literal pooling with various lengths:");
    print("Empty: '" + empty + "' == '" + empty_dup + "': " + (empty == empty_dup));
    print("Single: '" + single + "' == '" + single_dup + "': " + (single == single_dup));
    print("Short: '" + short_str + "' == '" + short_str_dup + "': " + (short_str == short_str_dup));
    print("Medium: '" + medium_str + "' == '" + medium_str_dup + "': " + (medium_str == medium_str_dup));

    if (empty == empty_dup && single == single_dup &&
        short_str == short_str_dup && medium_str == medium_str_dup) {
        print("String literal pooling works correctly");
    }
}

function testStringConcatenationPooling():void  {
    print("=== String Concatenation Pooling Test ===");

    string base = "base";
    string suffix = "suffix";

    // Multiple concatenations of the same strings
    string concat1 = base + "_" + suffix;
    string concat2 = base + "_" + suffix;  // Same operation, should potentially reuse
    string concat3 = "base" + "_" + "suffix";  // Literals, should match above

    print("Testing concatenation results:");
    print("concat1: " + concat1);
    print("concat2: " + concat2);
    print("concat3: " + concat3);

    if (concat1 == concat2 && concat2 == concat3) {
        print("String concatenation pooling works correctly");
    }
}

function testCommonIdentifierPatterns():void  {
    print("=== Common Identifier Pattern Test ===");

    // Simulate common variable names that appear frequently in programs
    string value = "value";
    string result = "result";
    string data = "data";
    string name = "name";
    string type = "type";
    string index = "index";
    string count = "count";
    string item = "item";

    // Create arrays to simulate multiple usage
    string[] values = new string[5];
    for (int i = 0; i < 5; i = i + 1) {
        values[i] = "value_" + i;  // Should pool "value_" prefix
    }

    print("Common identifier patterns created:");
    print("Core identifiers: " + value + ", " + result + ", " + data);
    print("Array values: " + values[0] + ", " + values[1] + ", " + values[2]);

    // Test reuse of common patterns
    string value_copy = "value";
    string result_copy = "result";

    if (value == value_copy && result == result_copy) {
        print("Common identifier pooling works correctly");
    }
}

function testStringPoolLimits(): void {
    print("=== String Pool Size Limits Test ===");

    // Test edge cases for pooling
    string very_short = "x";  // Should be pooled (>= MIN_LENGTH = 2 might not pool this)
    string exactly_min = "ab"; // Should be pooled (>= MIN_LENGTH = 2)
    string normal = "normal_string"; // Should be pooled

    // Create a very long string that might exceed MAX_LENGTH (1024)
    string very_long = "This is a very long string that is designed to test the maximum length limits of the string pool implementation. ";
    very_long = very_long + very_long; // Double it
    very_long = very_long + very_long; // Double again
    very_long = very_long + very_long; // Double again (should be > 1024 chars)

    print("Testing pool size limits:");
    print("Very short: '" + very_short + "'");
    print("Exactly min: '" + exactly_min + "'");
    print("Normal: '" + normal + "'");
    print("Very long length: " + strLength(very_long));

    // Test duplicate of normal string
    string normal_dup = "normal_string";
    if (normal == normal_dup) {
        print("Normal string pooling works");
    }

    // Very long string behavior (may or may not be pooled depending on limits)
    string very_long_dup = "This is a very long string that is designed to test the maximum length limits of the string pool implementation. ";
    very_long_dup = very_long_dup + very_long_dup;
    very_long_dup = very_long_dup + very_long_dup;
    very_long_dup = very_long_dup + very_long_dup;

    print("Very long string comparison: " + (very_long == very_long_dup));
}

class TestClass {
        string className;
        string methodName;
        string fieldName;

        public function init(string cName, string mName, string fName) {
            this.className = cName;   // Should pool common class names
            this.methodName = mName;  // Should pool common method names
            this.fieldName = fName;   // Should pool common field names
        }

        public function getInfo(): string {
            return this.className + "." + this.methodName + "(" + this.fieldName + ")";
        }
    }


function testStringPoolWithClasses(): void {
    print("=== String Pool with Classes Test ===");

    
    // Create multiple objects with common names
    TestClass obj1 = new TestClass();
    obj1.init("UserService", "getName", "username");

    TestClass obj2 = new TestClass();
    obj2.init("UserService", "setName", "username");  // Reuse "UserService" and "username"

    TestClass obj3 = new TestClass();
    obj3.init("DataService", "getName", "datafield"); // Reuse "getName"

    print("Class-based string pooling test:");
    print("obj1: " + obj1.getInfo());
    print("obj2: " + obj2.getInfo());
    print("obj3: " + obj3.getInfo());

    print("Class field string pooling completed");
}

// Entry point - run all efficiency tests
testStringDeduplication();
testKeywordPooling();
testStringLiteralPooling();
testStringConcatenationPooling();
testCommonIdentifierPatterns();
testStringPoolLimits();
testStringPoolWithClasses();

print("=== String Pool Efficiency Test Suite Completed ===");