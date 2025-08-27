// Switch statement extreme cases test

// Test 1: Switch with no cases (only default)
int test1 = 42;
switch (test1) {
    default:
        print("Only default case");
}

// Test 2: Switch with many cases (stress test)
int test2 = 15;
switch (test2) {
    case 1: print(1);
    case 2: print(2);
    case 3: print(3);
    case 4: print(4);
    case 5: print(5);
    case 6: print(6);
    case 7: print(7);
    case 8: print(8);
    case 9: print(9);
    case 10: print(10);
    case 11: print(11);
    case 12: print(12);
    case 13: print(13);
    case 14: print(14);
    case 15: print(15);  // This should match
    case 16: print(16);
    case 17: print(17);
    case 18: print(18);
    case 19: print(19);
    case 20: print(20);
    default: print("Default in many cases");
}

// Test 3: Switch with duplicate case values (if supported, should be an error in most languages)
// But testing if interpreter handles it gracefully
int test3 = 5;
switch (test3) {
    case 1: print("First 1");
    case 5: print("First 5");
    case 5: print("Duplicate 5 - should this work?");  
    case 10: print("Ten");
    default: print("Default for duplicates");
}

// Test 4: Switch without default case
int test4 = 999;
switch (test4) {
    case 1: print("One");
    case 2: print("Two");
    case 3: print("Three");
    // No default - should fall through without executing anything
}
print("After switch without default");

// Test 5: Empty switch (no cases, no default)
int test5 = 100;
switch (test5) {
    // Completely empty
}
print("After empty switch");

// Test 6: Switch with negative case values
int test6 = -5;
switch (test6) {
    case -10: print("Negative ten");
    case -5: print("Negative five");  // Should match
    case 0: print("Zero");
    case 5: print("Positive five");
    default: print("Default for negative test");
}

// Test 7: Switch with very large numbers
int test7 = 1000000;
switch (test7) {
    case 999999: print("Almost a million");
    case 1000000: print("Exactly one million"); // Should match
    case 1000001: print("Over a million");
    default: print("Default for large numbers");
}

// Test 8: Switch with expressions in case values (if supported)
int base = 10;
int test8 = 25;
switch (test8) {
    case 5 * 3: print("5 * 3 = 15");
    case base + 15: print("base + 15 = 25");  // Should match if expressions in cases are supported
    case 30: print("Thirty");
    default: print("Default for expression cases");
}

// Test 9: Nested switches
int outer = 1;
int inner = 2;
switch (outer) {
    case 1:
        print("Outer case 1");
        switch (inner) {
            case 1: print("Inner case 1");
            case 2: print("Inner case 2"); // Should match
            default: print("Inner default");
        }
    case 2:
        print("Outer case 2");
    default:
        print("Outer default");
}

// Test 10: Switch with boolean-like integer values
int test10 = 0;
switch (test10) {
    case 0: print("False-like (0)"); // Should match
    case 1: print("True-like (1)");
    default: print("Neither 0 nor 1");
}

// Test 11: Switch where all cases fall through (cascade effect)
int test11 = 1;
switch (test11) {
    case 1: 
        print("Case 1 - falling through");
    case 2: 
        print("Case 2 - falling through");
    case 3: 
        print("Case 3 - falling through");
    default: 
        print("Default - end of cascade");
}

print("All extreme switch tests completed");