// StringPool Stress Test
// Tests string pool behavior under heavy load and edge conditions

function testMassiveStringCreation(): void {
    print("=== Massive String Creation Stress Test ===");

    // Create many strings with repeated patterns
    string[] commonStrings = new string[100];

    for (int i = 0; i < 100; i = i + 1) {
        if (i < 25) {
            commonStrings[i] = "common_pattern_1";  // 25 identical strings
        } else if (i < 50) {
            commonStrings[i] = "common_pattern_2";  // 25 more identical strings
        } else if (i < 75) {
            commonStrings[i] = "unique_" + i; // 25 unique strings
        } else {
            commonStrings[i] = "common_pattern_1";  // Reuse first pattern
        }
    }

    print("Created 100 strings with repeated patterns");
    print("First few: " + commonStrings[0] + ", " + commonStrings[1] + ", " + commonStrings[2]);
    print("Middle: " + commonStrings[50] + ", " + commonStrings[51] + ", " + commonStrings[52]);
    print("End: " + commonStrings[97] + ", " + commonStrings[98] + ", " + commonStrings[99]);

    // Verify pooling effectiveness
    int matchCount = 0;
    for (int i = 0; i < 50; i = i + 1) {
        if (commonStrings[i] == commonStrings[i + 50]) {
            matchCount = matchCount + 1;
        }
    }

    print("Pattern matches found: " + matchCount);
    print("Massive string creation test completed");
}

function testStringConcatenationStress(): void {
    print("=== String Concatenation Stress Test ===");

    string base = "base";
    string result = base;

    // Perform many concatenations
    for (int i = 0; i < 20; i = i + 1) {
        result = result + "_" + i;
    }

    print("Built long string through concatenation:");
    print("Result length: " + strLength(result));
    print("Result starts with: " + result + "...");

    // Test multiple similar concatenation patterns
    string pattern1 = "start";
    string pattern2 = "start";

    for (int i = 0; i < 10; i = i + 1) {
        pattern1 = pattern1 + "_a";
        pattern2 = pattern2 + "_a";  // Same pattern, test pool reuse
    }

    if (pattern1 == pattern2) {
        print("Repeated concatenation patterns match correctly");
    } else {
        print("Concatenation patterns don't match (expected for unique results)");
    }

    print("String concatenation stress test completed");
}

function buildStringRecursively(int depth) : string {
        if (depth <= 0) {
            return "base";
        }
        return buildStringRecursively(depth - 1) + "_" + depth;
    }

function testRecursiveStringOperations(): void {
    print("=== Recursive String Operations Test ===");

    

    string result1 = buildStringRecursively(10);
    string result2 = buildStringRecursively(10);  // Same recursion pattern

    print("Recursive string building:");
    print("Result 1: " + result1);
    print("Result 2: " + result2);
    print("Results match: " + (result1 == result2));

    if (result1 == result2) {
        print("Recursive string operations produce consistent results");
    }

    print("Recursive string operations test completed");
}

function testStringPoolWithLoops(): void {
    print("=== String Pool with Loops Test ===");

    // Test nested loops with string operations
    for (int i = 0; i < 5; i = i + 1) {
        for (int j = 0; j < 5; j = j + 1) {
            string loopStr = "loop_" + i + "_" + j;

            // Use the string in some operation
            if (i == j) {
                string diagonal = "diagonal_" + i;
                string combined = loopStr + "=" + diagonal;
                print("Diagonal case: " + combined);
            }
        }
    }

    // Test repeated loop patterns
    string[] loopResults = new string[10];
    for (int k = 0; k < 10; k = k + 1) {
        loopResults[k] = "iteration_" + k;
    }

    // Verify some results
    print("Loop results sample:");
    print(loopResults[0] + ", " + loopResults[5] + ", " + loopResults[9]);

    print("String pool with loops test completed");
}

function testStringComparisonPerformance(): void {
    print("=== String Comparison Performance Test ===");

    // Create strings that should be pooled
    string template_str = "performance_test_template";
    string[] testStrings = new string[50];

    for (int i = 0; i < 50; i = i + 1) {
        if (i < 25) {
            testStrings[i] = template_str;  // Half should be pooled
        } else {
            testStrings[i] = "unique_string_" + i;
        }
    }

    // Perform many comparisons (should be fast with pooling)
    int equalCount = 0;
    int totalComparisons = 0;

    for (int i = 0; i < 50; i = i + 1) {
        for (int j = i + 1; j < 50; j = j + 1) {
            totalComparisons = totalComparisons + 1;
            if (testStrings[i] == testStrings[j]) {
                equalCount = equalCount + 1;
            }
        }
    }

    print("Comparison performance test results:");
    print("Total comparisons: " + totalComparisons);
    print("Equal pairs found: " + equalCount);
    print("String comparison performance test completed");
}

function testEdgeCaseStrings(): void {
    print("=== Edge Case Strings Test ===");

    // Test various edge cases
    string empty = "";
    string space = " ";
    string tab = "\t";
    string newline = "\n";
    string quote = "\"";
    string backslash = "\\";

    // Test special character combinations
    string mixed = "a\tb\nc\"d\\e";
    string unicode = "ñáéíóú";  // If supported
    string numbers = "1234567890";
    string symbols = "!@#$%^&*()";

    print("Edge case string testing:");
    print("Empty: '" + empty + "' (length: " + strLength(empty) + ")");
    print("Space: '" + space + "' (length: " + strLength(space) + ")");
    print("Mixed: '" + mixed + "'");
    print("Numbers: '" + numbers + "'");
    print("Symbols: '" + symbols + "'");

    // Test duplicates of edge cases
    string empty_dup = "";
    string space_dup = " ";
    string numbers_dup = "1234567890";

    if (empty == empty_dup && space == space_dup && numbers == numbers_dup) {
        print("Edge case string pooling works correctly");
    }

    print("Edge case strings test completed");
}


// Create many temporary strings in a scope
    function createTemporaryStrings(): string {
        string[] temp = new string[20];
        for (int i = 0; i < 20; i = i + 1) {
            temp[i] = "temporary_string_" + i;
        }
        return temp[0];  // Return just one string
    }

function testStringPoolMemoryBehavior(): void {
    print("=== String Pool Memory Behavior Test ===");


    string result = createTemporaryStrings();
    print("Result from temporary strings: " + result);

    // Create persistent strings
    string[] persistent = new string[10];
    for (int i = 0; i < 10; i = i + 1) {
        persistent[i] = "persistent_" + i;
    }

    print("Persistent strings created: " + persistent[0] + "..." + persistent[9]);
    print("String pool memory behavior test completed");
}

// Entry point - run all stress tests
testMassiveStringCreation();
testStringConcatenationStress();
testRecursiveStringOperations();
testStringPoolWithLoops();
testStringComparisonPerformance();
testEdgeCaseStrings();
testStringPoolMemoryBehavior();

print("=== String Pool Stress Test Suite Completed ===");