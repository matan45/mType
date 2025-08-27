// Comprehensive ternary operator nesting limits and extreme cases

// Test 1: Deep nesting (5 levels) - Finding maximum of 6 values
print("Test 1: Deep nesting (5 levels)");
int v1 = 10;
int v2 = 25;
int v3 = 15;
int v4 = 30;
int v5 = 20;
int v6 = 12;

int deep_max = v1 > v2 ? 
    (v1 > v3 ? 
        (v1 > v4 ? 
            (v1 > v5 ? 
                (v1 > v6 ? v1 : v6) 
                : (v5 > v6 ? v5 : v6)) 
            : (v4 > v5 ? 
                (v4 > v6 ? v4 : v6) 
                : (v5 > v6 ? v5 : v6))) 
        : (v3 > v4 ? 
            (v3 > v5 ? 
                (v3 > v6 ? v3 : v6) 
                : (v5 > v6 ? v5 : v6)) 
            : (v4 > v5 ? 
                (v4 > v6 ? v4 : v6) 
                : (v5 > v6 ? v5 : v6)))) 
    : (v2 > v3 ? 
        (v2 > v4 ? 
            (v2 > v5 ? 
                (v2 > v6 ? v2 : v6) 
                : (v5 > v6 ? v5 : v6)) 
            : (v4 > v5 ? 
                (v4 > v6 ? v4 : v6) 
                : (v5 > v6 ? v5 : v6))) 
        : (v3 > v4 ? 
            (v3 > v5 ? 
                (v3 > v6 ? v3 : v6) 
                : (v5 > v6 ? v5 : v6)) 
            : (v4 > v5 ? 
                (v4 > v6 ? v4 : v6) 
                : (v5 > v6 ? v5 : v6))));

print(deep_max);  // Should print 30

// Test 2: Extreme nesting (7 levels) - Binary decision tree
print("Test 2: Extreme nesting (7 levels)");
int input = 42;

int tree_result = input > 50 ?
    (input > 75 ?
        (input > 87 ?
            (input > 93 ?
                (input > 96 ?
                    (input > 98 ? 1 : 2)
                    : (input > 95 ? 3 : 4))
                : (input > 90 ?
                    (input > 91 ? 5 : 6)
                    : (input > 89 ? 7 : 8)))
            : (input > 81 ?
                (input > 84 ?
                    (input > 85 ? 9 : 10)
                    : (input > 83 ? 11 : 12))
                : (input > 78 ?
                    (input > 79 ? 13 : 14)
                    : (input > 77 ? 15 : 16))))
        : (input > 62 ?
            (input > 68 ?
                (input > 71 ?
                    (input > 73 ? 17 : 18)
                    : (input > 70 ? 19 : 20))
                : (input > 65 ?
                    (input > 66 ? 21 : 22)
                    : (input > 64 ? 23 : 24)))
            : (input > 56 ?
                (input > 59 ?
                    (input > 60 ? 25 : 26)
                    : (input > 58 ? 27 : 28))
                : (input > 53 ?
                    (input > 54 ? 29 : 30)
                    : (input > 52 ? 31 : 32)))))
    : (input > 25 ?
        (input > 37 ?
            (input > 43 ?
                (input > 46 ?
                    (input > 48 ? 33 : 34)
                    : (input > 45 ? 35 : 36))
                : (input > 40 ?
                    (input > 41 ? 37 : 38)
                    : (input > 39 ? 39 : 40)))
            : (input > 31 ?
                (input > 34 ?
                    (input > 35 ? 41 : 42)
                    : (input > 33 ? 43 : 44))
                : (input > 28 ?
                    (input > 29 ? 45 : 46)
                    : (input > 27 ? 47 : 48))))
        : (input > 12 ?
            (input > 18 ?
                (input > 21 ?
                    (input > 23 ? 49 : 50)
                    : (input > 20 ? 51 : 52))
                : (input > 15 ?
                    (input > 16 ? 53 : 54)
                    : (input > 14 ? 55 : 56)))
            : (input > 6 ?
                (input > 9 ?
                    (input > 10 ? 57 : 58)
                    : (input > 8 ? 59 : 60))
                : (input > 3 ?
                    (input > 4 ? 61 : 62)
                    : (input > 2 ? 63 : 64)))));

print(tree_result);  // Should print 37 (input=42)

// Test 3: Mixed data types in deep nesting
print("Test 3: Mixed data types in deep nesting");
bool flag1 = true;
bool flag2 = false;
int num1 = 100;
int num2 = 200;
float flt1 = 3.14;
float flt2 = 2.71;
string str1 = "hello";
string str2 = "world";

string mixed_result = flag1 ?
    (flag2 ?
        (num1 > num2 ? str1 : str2)
        : (flt1 > flt2 ?
            (num1 > 50 ? "big_num_pi" : "small_num_pi")
            : (num2 < 250 ? "medium_num_e" : "large_num_e")))
    : (num1 < 150 ?
        (flt2 > 2.0 ?
            (flag2 ? str2 : "false_branch")
            : (num2 > 180 ? "high_value" : "low_value"))
        : (flt1 < 4.0 ?
            (flag1 ? "true_float_check" : "false_float_check")
            : (num1 + num2 > 250 ? "sum_high" : "sum_low")));

print(mixed_result);  // Should print "big_num_pi"

// Test 4: Computational ternary chains (6 levels)
print("Test 4: Computational ternary chains");
int base = 10;

int computed = base > 0 ?
    (base % 2 == 0 ?
        (base > 5 ?
            (base < 15 ?
                (base * 2 > 15 ?
                    (base + 5 < 20 ? base * 3 : base * 4)
                    : (base - 2 > 5 ? base + 10 : base + 5))
                : (base % 3 == 1 ?
                    (base / 2 > 4 ? base - 3 : base - 1)
                    : (base + 1 < 20 ? base / 2 : base * 2)))
            : (base > 2 ?
                (base < 4 ?
                    (base == 3 ? 33 : 30)
                    : (base == 5 ? 55 : 50))
                : (base == 2 ?
                    (1 > 0 ? 22 : 20)
                    : (base == 1 ? 11 : 10))))
        : (base > 7 ?
            (base == 9 ?
                (base * 9 > 80 ? 999 : 99)
                : (base == 11 ? 1111 : 111))
            : (base < 6 ?
                (base == 5 ? 555 : 55)
                : (base == 7 ? 777 : 77))))
    : (base == 0 ?
        (0 == 0 ? 0 : -1)
        : (base > -5 ?
            (base == -1 ? -11 : -10)
            : (base < -10 ? -1000 : -100)));

print(computed);  // Should print 30 (base=10, even, >5, <15, 2*10>15=true, 10+5<20=true, so 10*3=30)

// Test 5: Boolean logic in extreme nesting (6 levels)
print("Test 5: Boolean logic in extreme nesting");
bool a = true;
bool b = false; 
bool c = true;
bool d = false;

bool complex_bool = a ?
    (b ?
        (c ? (d ? true : false) : (d ? false : true))
        : (c ?
            (d ? (a && b ? false : true) : (a || c ? true : false))
            : (d ?
                (!(a && b) ? (c || d ? true : false) : (!(c && d) ? false : true))
                : (!a ?
                    (b && c ? false : true)
                    : (c && !d ? true : false)))))
    : (b ?
        (c ?
            (d ? (a || b ? true : false) : (!a ? false : true))
            : (d ?
                (a && !b ? false : true)
                : (b || c ? true : false)))
        : (c ?
            (d ?
                (!(a || b) ? false : true)
                : (a && c ? true : false))
            : (d ? false : true)));

print(complex_bool);  // Should print true

// Test 6: String manipulation in deep nesting
print("Test 6: String manipulation in deep nesting");
string prefix = "test";
int counter = 3;

string str_result = counter > 0 ?
    (counter > 5 ?
        (counter > 10 ? 
            prefix + "_very_high"
            : (counter == 6 ? 
                prefix + "_six" 
                : prefix + "_high"))
        : (counter == 5 ?
            prefix + "_five"
            : (counter > 3 ?
                (counter == 4 ? 
                    prefix + "_four" 
                    : prefix + "_almost_five")
                : (counter == 3 ?
                    (prefix == "test" ? 
                        prefix + "_three_match" 
                        : prefix + "_three_nomatch")
                    : (counter == 2 ? 
                        prefix + "_two" 
                        : (counter == 1 ? 
                            prefix + "_one" 
                            : prefix + "_zero"))))))
    : (counter == 0 ?
        prefix + "_zero"
        : (counter > -3 ?
            prefix + "_small_negative"
            : prefix + "_large_negative"));

print(str_result);  // Should print "test_three_match"

// Test 7: Extreme arithmetic nesting (8 levels)
print("Test 7: Extreme arithmetic nesting");
int x = 15;
int y = 8;

int extreme_calc = x > y ?
    (x > y * 2 ?
        (x > y + 10 ?
            (x % y == 7 ?
                (x / y > 1 ?
                    (x - y > 5 ?
                        (x + y > 20 ?
                            (x * 2 > y * 3 ? x * 3 : x * 2)
                            : (x - y < 10 ? x + y : x - y))
                        : (y * 2 > x ?
                            (x + 2 > y ? y + x : y - x)
                            : (x / 2 > y / 2 ? x / 2 + y : x + y / 2)))
                    : (x + y > 15 ?
                        (x - 3 > y ? x - 3 + y : y - 3 + x)
                        : (y + 3 > x / 2 ? y + 3 : x / 2 + 3)))
                : (x * y > 100 ?
                    (x + y < 25 ? x + y + 10 : x + y - 10)
                    : (x - y > 0 ? (x - y) * 2 : (y - x) * 2)))
            : (x == y + 7 ? x + 7 : (x > y + 5 ? x + 5 : y + 5)))
        : (x < y + 8 ? x + y + 1 : x + y - 1))
    : (y > x - 2 ? y + x / 2 : x + y / 2);

print(extreme_calc);  // Should trace through: x=15, y=8, x>y=true, x>16=false, x<16=true, so 15+8+1=24

// Test 8: Conditional chains with different operators
print("Test 8: Conditional chains with different operators");
float val = 7.5;

float chain_result = val > 5.0 ?
    (val >= 7.0 ?
        (val <= 8.0 ?
            (val != 7.5 ? val + 1.0 : val * 2.0)
            : (val == 9.0 ? val / 2.0 : val - 1.0))
        : (val < 6.5 ?
            (val > 6.0 ? val + 0.5 : val - 0.5)
            : (val >= 6.5 ? val * 1.5 : val / 1.5)))
    : (val <= 3.0 ?
        (val >= 2.0 ? val + 2.0 : val + 3.0)
        : (val < 4.5 ?
            (val > 4.0 ? val - 2.0 : val - 1.0)
            : (val == 5.0 ? val : val + 1.0)));

print(chain_result);  // Should print 15.0 (val=7.5, matches val!=7.5 false, so val*2.0=15.0)

// Test 9: Maximum nesting depth stress test (6 levels)
print("Test 9: Maximum nesting depth stress test");
int depth_test = 5;

int max_depth = depth_test == 0 ? 0 :
    (depth_test == 1 ? 1 :
        (depth_test == 2 ? 2 :
            (depth_test == 3 ? 3 :
                (depth_test == 4 ? 4 :
                    (depth_test == 5 ? 555 : 10)))));

print(max_depth);  // Should print 555

// Test 10: Ternary with function calls (5 levels)
print("Test 10: Ternary with function calls");

function isEven(int n): bool {
    return n % 2 == 0;
}

function square(int n): int {
    return n * n;
}

function abs(int n): int {
    return n < 0 ? -n : n;
}

int func_input = 6;

int func_result = isEven(func_input) ?
    (square(func_input) > 30 ?
        (abs(func_input - 10) < 5 ?
            (func_input > square(2) ?
                (func_input + square(2) == 10 ? 1000 : 2000)
                : (abs(func_input) == func_input ? 3000 : 4000))
            : (square(func_input) > square(5) ?
                (isEven(square(func_input)) ? 5000 : 6000)
                : (func_input * 3 > 15 ? 7000 : 8000)))
        : (func_input < 5 ?
            (square(func_input) < 20 ? 9000 : 10000)
            : (abs(func_input - 3) > 2 ? 11000 : 12000)))
    : (func_input > 0 ?
        (square(func_input) < 50 ? -1000 : -2000)
        : (abs(func_input) > 5 ? -3000 : -4000));

print(func_result);  // func_input=6, isEven(6)=true, square(6)=36>30, abs(6-10)=4<5, 6>4, 6+4!=10, so 2000

// Test 11: Edge cases with boundary values
print("Test 11: Edge cases with boundary values");
int boundary = 0;

int boundary_result = boundary == 0 ?
    (boundary + 1 > 0 ?
        (boundary - 1 < 0 ?
            (boundary * 10 == 0 ?
                (boundary / 1 == 0 ? 100 : 101)
                : (boundary + 0 == 0 ? 102 : 103))
            : (boundary >= 0 ? 104 : 105))
        : (boundary <= 0 ? 106 : 107))
    : (boundary > 0 ?
        (boundary < 10 ? 200 : 201)
        : (boundary > -10 ? 202 : 203));

print(boundary_result);  // Should print 100

// Test 12: Performance stress with repetitive patterns (simplified)
print("Test 12: Performance stress with repetitive patterns");
int perf_val = 7;

int perf_result = perf_val > 0 ?
    (perf_val > 1 ? 
        (perf_val > 2 ? 
            (perf_val > 3 ? 
                (perf_val > 4 ? 
                    (perf_val > 5 ? 
                        (perf_val > 6 ? 
                            (perf_val > 7 ? 777 : 666) 
                        : 555) 
                    : 444) 
                : 333) 
            : 222) 
        : 111) 
    : 0) : 5;

print(perf_result);  // Should print 777

print("All ternary operator nesting limit tests completed");