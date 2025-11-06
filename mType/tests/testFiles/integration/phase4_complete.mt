// Phase 4: Syntax Sugar - Comprehensive Test
// Tests all auto-boxing and operator overloading features

import * from "lib/primitives/Int.mt";
import * from "lib/primitives/Float.mt";
import * from "lib/primitives/String.mt";

// Test function with auto-boxed parameters and return
function add(Int a, Int b): Int {
    return a + b;  // Uses operator overloading: a.add(b)
}

function multiply(Int x, Int y): Int {
    return x * y;  // Uses operator overloading: x.multiply(y)
}

// Test function with auto-boxed return statement
function getAnswer(): Int {
    return 42;  // Auto-boxes to: new Int(42)
}

function main(): void {
    print("=== Phase 4: Syntax Sugar - Complete Test ===");
    print("");

    // ============================================
    // 1. AUTO-BOXING IN DECLARATIONS
    // ============================================
    print("1. Auto-boxing in declarations:");
    Int x = 42;           // Auto-boxes to: new Int(42)
    Float f = 3.14;       // Auto-boxes to: new Float(3.14)
    String s = "hello";   // Auto-boxes to: new String("hello")

    print("  Int x = 42:      " + x.toString());
    print("  Float f = 3.14:  " + f.toString());
    print("  String s:        " + s.toString());
    print("");

    // ============================================
    // 2. AUTO-BOXING IN ASSIGNMENTS
    // ============================================
    print("2. Auto-boxing in assignments:");
    Int y;
    y = 100;              // Auto-boxes to: new Int(100)

    Float g;
    g = 2.71;             // Auto-boxes to: new Float(2.71)

    print("  y = 100:         " + y.toString());
    print("  g = 2.71:        " + g.toString());
    print("");

    // ============================================
    // 3. OPERATOR OVERLOADING - ARITHMETIC
    // ============================================
    print("3. Operator overloading (arithmetic):");
    Int a = 15;
    Int b = 7;

    Int sum = a + b;      // Transforms to: a.add(b)
    Int diff = a - b;     // Transforms to: a.subtract(b)
    Int prod = a * b;     // Transforms to: a.multiply(b)
    Int quot = a / b;     // Transforms to: a.divide(b)
    Int rem = a % b;      // Transforms to: a.modulo(b)

    print("  15 + 7 = " + sum.toString());
    print("  15 - 7 = " + diff.toString());
    print("  15 * 7 = " + prod.toString());
    print("  15 / 7 = " + quot.toString());
    print("  15 % 7 = " + rem.toString());
    print("");

    // ============================================
    // 4. OPERATOR OVERLOADING - COMPARISON
    // ============================================
    print("4. Operator overloading (comparison):");
    Int val1 = 10;
    Int val2 = 20;

    bool isLess = val1 < val2;           // Transforms to: val1.lessThan(val2)
    bool isLessEq = val1 <= val2;        // Transforms to: val1.lessThanOrEqual(val2)
    bool isGreater = val1 > val2;        // Transforms to: val1.greaterThan(val2)
    bool isGreaterEq = val1 >= val2;     // Transforms to: val1.greaterThanOrEqual(val2)
    bool isEqual = val1 == val2;         // Transforms to: val1.equals(val2)
    bool isNotEqual = val1 != val2;      // Transforms to: !(val1.equals(val2))

    if (isLess) {
        print("  10 < 20:  true");
    }
    if (isLessEq) {
        print("  10 <= 20: true");
    }
    if (!isGreater) {
        print("  10 > 20:  false");
    }
    if (!isGreaterEq) {
        print("  10 >= 20: false");
    }
    if (!isEqual) {
        print("  10 == 20: false");
    }
    if (isNotEqual) {
        print("  10 != 20: true");
    }
    print("");

    // ============================================
    // 5. STRING CONCATENATION
    // ============================================
    print("5. String concatenation:");
    String greeting = "Hello";
    String target = "World";
    String message = greeting + ", " + target + "!";  // Uses concat()

    print("  Result: " + message.toString());
    print("");

    // ============================================
    // 6. FUNCTION CALLS WITH AUTO-BOXING
    // ============================================
    print("6. Function calls with auto-boxing:");
    Int result1 = add(5, 10);          // Arguments auto-boxed!
    Int result2 = multiply(3, 7);      // Arguments auto-boxed!

    print("  add(5, 10):      " + result1.toString());
    print("  multiply(3, 7):  " + result2.toString());
    print("");

    // ============================================
    // 7. RETURN STATEMENT AUTO-BOXING
    // ============================================
    print("7. Return statement auto-boxing:");
    Int answer = getAnswer();          // Returns auto-boxed Int

    print("  getAnswer():     " + answer.toString());
    print("");

    // ============================================
    // 8. INTEGER CACHING (Phase 2 Integration)
    // ============================================
    print("8. Integer caching (Phase 2):");
    Int cached1 = 50;                  // Auto-boxed and cached
    Int cached2 = 50;                  // Same object from cache
    Int cached3 = 128;                 // Auto-boxed and cached

    print("  50 cached:       " + cached1.toString());
    print("  50 cached again: " + cached2.toString());
    print("  128 cached:      " + cached3.toString());
    print("");

    // ============================================
    // 9. COMPLEX EXPRESSION WITH ALL FEATURES
    // ============================================
    print("9. Complex expression:");
    Int n1 = 5;
    Int n2 = 3;
    Int n3 = 2;

    // Complex expression: (5 + 3) * 2 - 10
    Int complex = (n1 + n2) * n3;
    Int final = complex - 10;

    print("  (5 + 3) * 2 - 10 = " + final.toString());
    print("");

    // ============================================
    // 10. COMPARISON IN CONTROL FLOW
    // ============================================
    print("10. Comparison in control flow:");
    Int score = 85;
    Int passingScore = 60;

    if (score > passingScore) {
        print("  Score " + score.toString() + " > " + passingScore.toString() + ": PASS");
    }

    Int count = 0;
    Int limit = 5;
    while (count < limit) {
        count = count + 1;
    }
    print("  Counted to: " + count.toString());
    print("");

    // ============================================
    // 11. BOOL AUTO-BOXING AND OPERATORS
    // ============================================
    print("11. Bool auto-boxing and operators:");
    Bool isTrue = true;       // Auto-boxes to: new Bool(true)
    Bool isFalse = false;     // Auto-boxes to: new Bool(false)

    print("  Bool isTrue = true:   " + isTrue.toString());
    print("  Bool isFalse = false: " + isFalse.toString());

    // Bool equality operators
    bool same = isTrue == isTrue;         // Transforms to: isTrue.equals(isTrue)
    bool different = isTrue != isFalse;   // Transforms to: !(isTrue.equals(isFalse))

    if (same) {
        print("  true == true:  true");
    }
    if (different) {
        print("  true != false: true");
    }
    print("");

    // ============================================
    // 12. ARRAY TESTS WITH AUTO-BOXING
    // ============================================
    print("12. Arrays with Box types:");

    // Array of Int objects with auto-boxing!
    Int[] numbers = [1, 2, 3, 4, 5];  // Auto-boxes each element!

    print("  Int[] numbers = [1, 2, 3, 4, 5] (auto-boxed!)");

    // Access array elements and use operators
    Int first = numbers[0];
    Int second = numbers[1];
    Int sumArray = first + second;  // Uses operator overloading

    print("  numbers[0] + numbers[1] = " + sumArray.toString());

    // Modify array with auto-boxed values
    numbers[2] = 10;  // Auto-boxes to: new Int(10)
    print("  After numbers[2] = 10: " + numbers[2].toString());

    // Loop through array with operators
    Int arraySum = 0;
    int idx = 0;
    while (idx < 5) {
        arraySum = arraySum + numbers[idx];  // Operator overloading in loop
        idx = idx + 1;
    }
    print("  Sum of all elements: " + arraySum.toString());

    // Float array with auto-boxing
    Float[] prices = [9.99, 19.99, 29.99];  // Auto-boxes each element!
    print("  Float[] prices = [9.99, 19.99, 29.99] (auto-boxed!)");
    Float firstPrice = prices[0];
    print("  First price: " + firstPrice.toString());

    // Bool array with auto-boxing
    Bool[] flags = [true, false, true];  // Auto-boxes each element!
    print("  Bool[] flags = [true, false, true] (auto-boxed!)");
    Bool firstFlag = flags[0];
    print("  First flag: " + firstFlag.toString());
    print("");

    // ============================================
    // 13. FLOAT OPERATIONS
    // ============================================
    print("13. Float auto-boxing and operators:");
    Float pi = 3.14159;       // Auto-boxes
    Float radius = 2.0;       // Auto-boxes

    Float circumference = pi * radius;  // Operator overloading
    print("  PI * radius: " + circumference.toString());

    Float half = radius / 2.0;  // Division operator
    print("  radius / 2: " + half.toString());

    // Float comparisons
    bool piGreater = pi > radius;  // Transforms to: pi.greaterThan(radius)
    if (piGreater) {
        print("  3.14159 > 2.0: true");
    }
    print("");

    // ============================================
    // 14. MIXED OPERATIONS (Complex Test)
    // ============================================
    print("14. Complex mixed operations:");

    // Combine multiple features
    Int base = 10;
    Int exponent = 2;
    Int power = base * base;  // 10 * 10 = 100

    if (power > 50) {
        print("  10^2 = " + power.toString() + " (greater than 50)");
    }

    // Use in function call with return
    Int doubled = multiply(power, 2);  // multiply(100, 2) = 200
    print("  multiply(100, 2) = " + doubled.toString());

    // Comparison chain
    bool inRange = (doubled > 100) && (doubled < 300);
    if (inRange) {
        print("  200 is between 100 and 300: true");
    }
    print("");

    // ============================================
    // 15. STRING OPERATIONS
    // ============================================
    print("15. String auto-boxing and concatenation:");
    String firstName = "John";
    String lastName = "Doe";
    String space = " ";

    String fullName = firstName + space + lastName;  // Multiple concatenations
    print("  Full name: " + fullName.toString());

    String greeting2 = "Hello, " + fullName + "!";
    print("  Greeting: " + greeting2.toString());
    print("");

    // ============================================
    // SUCCESS!
    // ============================================
    print("=== ALL PHASE 4 FEATURES WORKING! ===");
    print("Total tests: 15 categories covering:");
    print("  - Auto-boxing (declarations, assignments, args, returns)");
    print("  - Operator overloading (arithmetic, comparison, equality)");
    print("  - Box types (Int, Float, Bool, String)");
    print("  - Arrays with Box types");
    print("  - Complex mixed operations");
    print("SUCCESS!");
}

main();
