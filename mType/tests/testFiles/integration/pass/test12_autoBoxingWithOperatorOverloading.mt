// Integration Test 12: Auto-Boxing with Operator Overloading
// Tests: All auto-boxing types + Arithmetic/comparison operators

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Main test execution
print("=== Test 12: Auto-Boxing with Operator Overloading ===");

// Test 1: Int auto-boxing and arithmetic operators
print("--- Int auto-boxing and operators ---");

Int a = 10;  // Auto-boxes to new Int(10)
Int b = 20;  // Auto-boxes to new Int(20)

print("a = " + a.getValue());
print("b = " + b.getValue());

// Addition
Int sum = a + b;  // Calls a.add(b)
print("a + b = " + sum.getValue());

// Subtraction
Int diff = b - a;  // Calls b.subtract(a)
print("b - a = " + diff.getValue());

// Multiplication
Int prod = a * b;  // Calls a.multiply(b)
print("a * b = " + prod.getValue());

// Division
Int quot = b / a;  // Calls b.divide(a)
print("b / a = " + quot.getValue());

// Modulo
Int c = 17;
Int d = 5;
Int rem = c % d;  // Calls c.modulo(d)
print("17 % 5 = " + rem.getValue());

// Test 2: Int comparison operators
print("--- Int comparison operators ---");

Int x = 15;
Int y = 25;

Bool isLess = x < y;  // Calls x.lessThan(y)
print("15 < 25: " + isLess.getValue());

Bool isLessEq = x <= x;  // Calls x.lessThanOrEqual(x)
print("15 <= 15: " + isLessEq.getValue());

Bool isGreater = y > x;  // Calls y.greaterThan(x)
print("25 > 15: " + isGreater.getValue());

Bool isGreaterEq = y >= y;  // Calls y.greaterThanOrEqual(y)
print("25 >= 25: " + isGreaterEq.getValue());

Bool isEqual = x == x;  // Calls x.equals(x)
print("15 == 15: " + isEqual.getValue());

Bool isNotEqual = x != y;  // Calls !(x.equals(y))
print("15 != 25: " + isNotEqual.getValue());

// Test 3: Float auto-boxing and operators
print("--- Float auto-boxing and operators ---");

Float f1 = 10.5;  // Auto-boxes to new Float(10.5)
Float f2 = 2.5;   // Auto-boxes to new Float(2.5)

print("f1 = " + f1.getValue());
print("f2 = " + f2.getValue());

Float fSum = f1 + f2;
print("f1 + f2 = " + fSum.getValue());

Float fDiff = f1 - f2;
print("f1 - f2 = " + fDiff.getValue());

Float fProd = f1 * f2;
print("f1 * f2 = " + fProd.getValue());

Float fQuot = f1 / f2;
print("f1 / f2 = " + fQuot.getValue());

// Float comparisons
Bool fLess = f2 < f1;
print("2.5 < 10.5: " + fLess.getValue());

Bool fEqual = f1 == f1;
print("10.5 == 10.5: " + fEqual.getValue());

// Test 4: String auto-boxing and concatenation
print("--- String auto-boxing and operators ---");

String s1 = "Hello";   // Auto-boxes to new String("Hello")
String s2 = " World";  // Auto-boxes to new String(" World")

print("s1 = " + s1.getValue());
print("s2 = " + s2.getValue());

String concat = s1 + s2;  // Calls s1.concat(s2)
print("s1 + s2 = " + concat.getValue());

Bool strEqual = s1 == s1;  // Calls s1.equals(s1)
print("Hello == Hello: " + strEqual.getValue());

Bool strNotEqual = s1 != s2;
print("Hello != World: " + strNotEqual.getValue());

// Test 5: Bool auto-boxing
print("--- Bool auto-boxing ---");

Bool bool1 = true;   // Auto-boxes to new Bool(true)
Bool bool2 = false;  // Auto-boxes to new Bool(false)

print("bool1 = " + bool1.getValue());
print("bool2 = " + bool2.getValue());

Bool boolEqual = bool1 == bool1;
print("true == true: " + boolEqual.getValue());

Bool boolNotEqual = bool1 != bool2;
print("true != false: " + boolNotEqual.getValue());

// Test 6: Mixed operations and chaining
print("--- Mixed operations ---");

Int m1 = 5;
Int m2 = 10;
Int m3 = 15;

Int chain = (m1 + m2) * m3;  // (5 + 10) * 15 = 225
print("(5 + 10) * 15 = " + chain.getValue());

Int chain2 = m3 - (m2 / m1);  // 15 - (10 / 5) = 13
print("15 - (10 / 5) = " + chain2.getValue());

// Test 7: Auto-boxing in function calls
print("--- Auto-boxing in function calls ---");

function add(Int a, Int b): Int {
    return a + b;
}

Int result1 = add(30, 40);  // Both auto-boxed to Int
print("add(30, 40) = " + result1.getValue());

// Test 8: Auto-boxing in arrays
print("--- Auto-boxing in arrays ---");

Int[] numbers = [1, 2, 3, 4, 5];  // Each element auto-boxed

print("Array elements:");
for (int i = 0; i < numbers.length; i = i + 1) {
    print("numbers[" + i + "] = " + numbers[i].getValue());
}

// Array element operations
Int arraySum = numbers[0] + numbers[1];
print("numbers[0] + numbers[1] = " + arraySum.getValue());

// Test 9: Comparison chains
print("--- Comparison chains ---");

Int n1 = 10;
Int n2 = 20;
Int n3 = 30;

Bool comp1 = n1 < n2;
Bool comp2 = n2 < n3;

print("10 < 20: " + comp1.getValue());
print("20 < 30: " + comp2.getValue());

if (comp1.getValue() && comp2.getValue()) {
    print("Both comparisons true");
}

// Test 10: Negative numbers and edge cases
print("--- Negative numbers and edge cases ---");

Int neg1 = -10;
Int neg2 = -5;

Int negSum = neg1 + neg2;
print("-10 + -5 = " + negSum.getValue());

Int negProd = neg1 * neg2;
print("-10 * -5 = " + negProd.getValue());

Bool negComp = neg2 > neg1;
print("-5 > -10: " + negComp.getValue());

// Zero operations
Int zero = 0;
Int zeroSum = zero + zero;
print("0 + 0 = " + zeroSum.getValue());

Bool zeroEqual = zero == zero;
print("0 == 0: " + zeroEqual.getValue());

print("=== Test 12 Complete ===");
