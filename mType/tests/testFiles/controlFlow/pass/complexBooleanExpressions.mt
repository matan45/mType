// Comprehensive complex boolean expression evaluation with edge cases

// Test 1: Basic logical operators
print("Test 1: Basic logical operators");
bool a = true;
bool b = false;
bool c = true;

bool and_result = a && b;  // false
bool or_result = a || b;   // true
bool not_result = !a;      // false

print(and_result);  // false
print(or_result);   // true
print(not_result);  // false

// Test 2: Complex AND chains
print("Test 2: Complex AND chains");
bool chain_and = true && true && true && true;
print(chain_and);  // true

chain_and = true && true && false && true;
print(chain_and);  // false

chain_and = false && true && true && true;
print(chain_and);  // false (short-circuit)

// Test 3: Complex OR chains  
print("Test 3: Complex OR chains");
bool chain_or = false || false || false || false;
print(chain_or);  // false

chain_or = false || false || true || false;
print(chain_or);  // true

chain_or = true || false || false || false;
print(chain_or);  // true (short-circuit)

// Test 4: Mixed AND/OR precedence
print("Test 4: Mixed AND/OR precedence");
bool mixed1 = true || false && false;  // true (AND has higher precedence)
print(mixed1);  // true

bool mixed2 = false && true || true;   // true
print(mixed2);  // true

bool mixed3 = true && false || false && true;  // false
print(mixed3);  // false

// Test 5: Parentheses override precedence
print("Test 5: Parentheses override precedence");
bool paren1 = (true || false) && false;  // false
print(paren1);  // false

bool paren2 = true || (false && false);  // true
print(paren2);  // true

bool paren3 = (false || true) && (true || false);  // true
print(paren3);  // true

// Test 6: Negation with complex expressions
print("Test 6: Negation with complex expressions");
bool neg1 = !(true && false);  // true
print(neg1);  // true

bool neg2 = !(false || false);  // true
print(neg2);  // true

bool neg3 = !(!true || !false);  // false
print(neg3);  // false

bool neg4 = !!true;  // true (double negation)
print(neg4);  // true

// Test 7: Comparison operations in boolean contexts
print("Test 7: Comparison operations");
int x = 10;
int y = 5;
int z = 10;

bool comp1 = x > y && y < z;  // true
print(comp1);  // true

bool comp2 = x == z || y > x;  // true
print(comp2);  // true

bool comp3 = x != y && x >= z && y <= z;  // true
print(comp3);  // true

// Test 8: Boolean expressions with arithmetic
print("Test 8: Boolean with arithmetic");
bool arith1 = (x + y) > z && (x - y) >= 0;  // true
print(arith1);  // true

bool arith2 = (x * 2) == (z + y + y) || (y / 5) == 1;  // true
print(arith2);  // true

bool arith3 = (x % 3) == 1 && (y % 2) == 1;  // true
print(arith3);  // true

// Test 9: Short-circuit evaluation testing (without division by zero)
print("Test 9: Short-circuit evaluation");
bool short1 = false && (x > 1000);  // Should short-circuit to false
print(short1);  // false

bool short2 = true || (x < 0);       // Should short-circuit to true
print(short2);  // true

// Test 10: Complex nested boolean expressions
print("Test 10: Complex nested expressions");
bool nested1 = ((true && false) || (true && true)) && !(false || false);
print(nested1);  // true

bool nested2 = ((!true || false) && true) || (!(false && true) && false);
print(nested2);  // false

bool nested3 = ((x > 0) && (y > 0)) || ((x < 0) && (y < 0)) || (x == y);
print(nested3);  // true

// Test 11: Boolean functions and expressions
print("Test 11: Boolean functions");
function isEven(int n): bool {
    return (n % 2) == 0;
}

function isPositive(int n): bool {
    return n > 0;
}

function isBetween(int val, int min, int max): bool {
    return val >= min && val <= max;
}

bool func1 = isEven(10) && isPositive(10);  // true
print(func1);  // true

bool func2 = isEven(5) || isPositive(-3);   // false
print(func2);  // false

bool func3 = isBetween(7, 5, 10) && !isEven(7);  // true
print(func3);  // true

// Test 12: Boolean expressions in control flow
print("Test 12: Boolean in control flow");
int count = 0;
for (int i = 1; i <= 10; i = i + 1) {
    if (isEven(i) && isPositive(i) && isBetween(i, 2, 8)) {
        count = count + 1;
    }
}
print(count);  // Should print 4 (2, 4, 6, 8)

// Test 13: Boolean with string comparisons
print("Test 13: Boolean with strings");
string str1 = "hello";
string str2 = "world";
string str3 = "hello";

bool str_comp1 = (str1 == str3) && (str1 != str2);  // true
print(str_comp1);  // true

// Test 14: Complex ternary with boolean expressions
print("Test 14: Complex ternary expressions");
int val = 15;
bool complex_ternary = val > 10 ? (val % 2 == 1 && val < 20) : (val >= 0 && val <= 10);
print(complex_ternary);  // true

string ternary_string = (val > 5 && val < 25) ? "in range" : "out of range";
print(ternary_string);  // "in range"

// Test 15: Boolean expressions with float comparisons
print("Test 15: Boolean with floats");
float f1 = 3.14;
float f2 = 2.71;
float f3 = 3.14;

bool float_comp = (f1 == f3) && (f1 > f2) && (f2 > 0.0);  // true
print(float_comp);  // true

bool float_range = (f1 >= 3.0) && (f1 <= 4.0) && (f2 >= 2.0) && (f2 <= 3.0);
print(float_range);  // true

// Test 16: Extreme boolean expression complexity
print("Test 16: Extreme complexity");
bool extreme = ((true && !false) || (false && true)) && 
               (!(false || false) && !!(true && true)) &&
               ((x > y) && (y > 0)) &&
               ((f1 > f2) || (f2 < 0.0)) &&
               (isEven(x) || isEven(y + 1)) &&
               isBetween(x + y, 10, 20);
print(extreme);  // true

// Test 17: Boolean expressions with method chaining simulation
print("Test 17: Method chaining simulation");
function checkValue(int v): bool {
    return v > 0 && v < 100 && v % 5 == 0;
}

function transformValue(int v): int {
    return v * 2 + 1;
}

bool chained = checkValue(transformValue(10)) || checkValue(transformValue(7));
print(chained);  // false (21 and 15, neither divisible by 5)

bool chained2 = checkValue(transformValue(12)) && isPositive(transformValue(12));
print(chained2);  // true (25 is divisible by 5 and positive)

// Test 18: Boolean expressions with loops and accumulation
print("Test 18: Boolean with loops");
bool any_even = false;
bool all_positive = true;

for (int i = 1; i <= 5; i = i + 1) {
    any_even = any_even || isEven(i);
    all_positive = all_positive && isPositive(i);
}
print(any_even);    // true (2, 4 are even)
print(all_positive); // true (all 1-5 are positive)

// Test 19: Boolean expressions with multiple data types
print("Test 19: Multiple data types");
int int_val = 42;
float float_val = 3.14;
string str_val = "test";
bool bool_val = true;

bool multi_type = (int_val > 40) && 
                  (float_val > 3.0) && 
                  (str_val == "test") && 
                  bool_val;
print(multi_type);  // true

// Test 20: Edge cases with zero and negative values
print("Test 20: Edge cases with zero/negative");
int zero = 0;
int negative = -5;
int positive = 7;

bool edge1 = (zero == 0) && (negative < 0) && (positive > 0);  // true
print(edge1);  // true

bool edge2 = !(zero != 0) && !(negative >= 0) && !(positive <= 0);  // true
print(edge2);  // true

// VFScript may not support int-to-bool conversion, so test with explicit comparisons
bool edge3 = (zero != 0) || (negative != 0) && (positive != 0);  // explicit bool conversion
print(edge3);  // true

print("Zero comparison tests");
bool zero_test = (zero == 0);
print(zero_test);  // true

// Test 21: Boolean expressions with array-like operations (using multiple variables)
print("Test 21: Array-like operations simulation");
int arr1 = 10;
int arr2 = 20; 
int arr3 = 30;
int target = 20;

bool found = (arr1 == target) || (arr2 == target) || (arr3 == target);  // true
print(found);  // true

bool all_greater_than_5 = (arr1 > 5) && (arr2 > 5) && (arr3 > 5);  // true
print(all_greater_than_5);  // true

// Test 22: Boolean expressions with mathematical functions
print("Test 22: Mathematical functions");
function isPrime(int n): bool {
    if (n <= 1) {
        return false;
    }
    if (n <= 3) {
        return true;
    }
    if (n % 2 == 0 || n % 3 == 0) {
        return false;
    }
    
    int i = 5;
    while (i * i <= n) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
        i = i + 6;
    }
    return true;
}

bool math1 = isPrime(17) && isPrime(13) && !isPrime(15);  // true
print(math1);  // true

bool math2 = isPrime(2) || isPrime(4) || isPrime(6);  // true (2 is prime)
print(math2);  // true

print("All complex boolean expression tests completed");