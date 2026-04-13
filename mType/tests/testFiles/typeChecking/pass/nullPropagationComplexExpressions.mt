// Comprehensive null propagation through complex expressions test
// Tests key scenarios where null values propagate through complex nested expressions

class TestData {
    public int value;
    public string name;

    constructor(int v, string n) {
        value = v;
        name = n;
    }

    public function getValue(): int {
        return value;
    }

    public function getName(): string {
        return name;
    }

    public function toString(): string {
        return name + "(" + value + ")";
    }
}

// Test 1: Null propagation in arithmetic expressions
print("Test 1: Null propagation in arithmetic expressions");
TestData? nullData = null;
TestData validData = new TestData(42, "valid");

// Safe arithmetic with null checks
int part1 = 0;
if (nullData != null) { part1 = nullData.getValue(); }
int part2 = 0;
if (validData != null) { part2 = validData.getValue(); }
int result1 = part1 + part2;
print("Safe arithmetic result: " + result1);  // Should be 42

// Nested null checks in arithmetic
int left2 = 10;
if (nullData != null) {
    left2 = nullData.getValue() > 0 ? nullData.getValue() * 2 : nullData.getValue();
}
int right2 = 20;
if (validData != null) {
    right2 = validData.getValue() < 50 ? validData.getValue() + 5 : validData.getValue();
}
int result2 = left2 + right2;
print("Nested null arithmetic result: " + result2);  // Should be 57 (10 + 47)

// Test 2: Null propagation in ternary chains
print("Test 2: Null propagation in ternary chains");
string ternaryResult = "";
if (nullData == null) {
    if (validData == null) {
        ternaryResult = "both null";
    } else if (validData.getValue() > 40) {
        ternaryResult = "valid high: " + validData.toString();
    } else {
        ternaryResult = "valid low: " + validData.toString();
    }
} else {
    if (nullData.getValue() > 0) {
        ternaryResult = "null positive";
    } else {
        ternaryResult = "null non-positive";
    }
}
print("Ternary chain result: " + ternaryResult);  // Should be "valid high: valid(42)"

// Deep ternary with null propagation
int deepTernary = 0;
if (nullData == null) {
    if (validData == null) {
        deepTernary = -999;
    } else if (validData.getValue() > 50) {
        deepTernary = 1;
    } else if (validData.getValue() > 40) {
        deepTernary = 2;
    } else if (validData.getValue() > 30) {
        deepTernary = 3;
    } else {
        deepTernary = 4;
    }
} else {
    if (nullData.getValue() == 0) {
        deepTernary = 0;
    } else if (nullData.getValue() > 0) {
        deepTernary = 100;
    } else {
        deepTernary = -100;
    }
}
print("Deep ternary result: " + deepTernary);  // Should be 2

// Test 3: Null propagation in function calls
print("Test 3: Null propagation in function calls");

function safeGetValue(TestData? data): int {
    if (data != null) { return data.getValue(); }
    return -1;
}

function safeGetName(TestData? data): string {
    if (data != null) { return data.getName(); }
    return "null";
}

int safeValue1 = safeGetValue(validData);
int safeValue2 = safeGetValue(nullData);
string safeName1 = safeGetName(validData);
string safeName2 = safeGetName(nullData);

print("Safe value 1: " + safeValue1);  // 42
print("Safe value 2: " + safeValue2);  // -1
print("Safe name 1: " + safeName1);              // "valid"
print("Safe name 2: " + safeName2);              // "null"

// Test 4: Null propagation in loops
print("Test 4: Null propagation in loops");
TestData item1 = new TestData(10, "first");
TestData? item2 = null;
TestData item3 = new TestData(30, "third");

for (int i = 0; i < 3; i++) {
    TestData? current = null;
    
    if (i == 0) current = item1;
    else if (i == 1) current = item2;
    else current = item3;
    
    string result = "Item " + i + ": null (invalid)";
    if (current != null) {
        result = "Item " + i + ": " + current.toString() + " (valid)";
    }
    
    print(result);
}

// Test 5: Null propagation in mixed data types
print("Test 5: Null propagation in mixed data types");
bool flag = true;
TestData? conditionalData = null;
if (flag) { conditionalData = validData; } else { conditionalData = nullData; }

string mixedResult = "flag false";
if (flag) {
    if (conditionalData != null) {
        if (conditionalData.getValue() > 0) {
            mixedResult = "positive: " + conditionalData.toString();
        } else {
            mixedResult = "non-positive: " + conditionalData.toString();
        }
    } else {
        mixedResult = "flag true but data null";
    }
}
print("Mixed result: " + mixedResult);  // Should be "positive: valid(42)"

// Test 6: Null propagation with mathematical operations
print("Test 6: Null propagation with mathematical operations");
function calculateSafeSum(TestData? data1, TestData? data2): int {
    int val1 = 0;
    if (data1 != null) { val1 = data1.getValue(); }
    int val2 = 0;
    if (data2 != null) { val2 = data2.getValue(); }
    return val1 + val2;
}

function calculateSafeProduct(TestData? data1, TestData? data2): int {
    int val1 = 1;
    if (data1 != null) { val1 = data1.getValue(); }
    int val2 = 1;
    if (data2 != null) { val2 = data2.getValue(); }
    return val1 * val2;
}

int sum1 = calculateSafeSum(validData, nullData);
int sum2 = calculateSafeSum(nullData, nullData);
int product1 = calculateSafeProduct(validData, nullData);
int product2 = calculateSafeProduct(nullData, nullData);

print("Sum with one null: " + sum1);      // 42
print("Sum with both null: " + sum2);     // 0
print("Product with one null: " + product1); // 42
print("Product with both null: " + product2); // 1

// Test 7: Null propagation in string operations
print("Test 7: Null propagation in string operations");
function createDescription(TestData? data, string prefix): string {
    if (data != null) {
        return prefix + data.getName() + " has value " + data.getValue();
    }
    return prefix + "no data available";
}

string desc1 = createDescription(validData, "Result: ");
string desc2 = createDescription(nullData, "Result: ");

print(desc1);  // Result: valid has value 42
print(desc2);  // Result: no data available

// Test 8: Null propagation in nested conditionals
print("Test 8: Null propagation in nested conditionals");
function evaluateNested(TestData? outer, TestData? inner): string {
    if (outer != null) {
        if (inner != null) {
            if (outer.getValue() > inner.getValue()) {
                return outer.toString() + " > " + inner.toString();
            } else if (outer.getValue() < inner.getValue()) {
                return outer.toString() + " < " + inner.toString();
            } else {
                return outer.toString() + " = " + inner.toString();
            }
        }
        return outer.toString() + " vs null";
    }
    if (inner != null) {
        return "null vs " + inner.toString();
    }
    return "null vs null";
}

TestData data20 = new TestData(20, "twenty");
string nested1 = evaluateNested(validData, data20);
string nested2 = evaluateNested(nullData, data20);
string nested3 = evaluateNested(validData, nullData);
string nested4 = evaluateNested(nullData, nullData);

print("Nested 1: " + nested1);  // valid(42) > twenty(20)
print("Nested 2: " + nested2);  // null vs twenty(20)
print("Nested 3: " + nested3);  // valid(42) vs null
print("Nested 4: " + nested4);  // null vs null

// Test 9: Complex null propagation chains
print("Test 9: Complex null propagation chains");
function chainedOperations(TestData? data): int {
    if (data != null) {
        if (data.getValue() > 0) {
            if (data.getValue() < 100) {
                return data.getValue() * 2;
            }
            return data.getValue();
        }
        return 0;
    }
    return -1;
}

int chain1 = chainedOperations(validData);   // 84 (42 * 2)
int chain2 = chainedOperations(nullData);    // -1
int chain3 = chainedOperations(new TestData(150, "large")); // 150
int chain4 = chainedOperations(new TestData(-10, "negative")); // 0

print("Chain 1: " + chain1);  // 84
print("Chain 2: " + chain2);  // -1
print("Chain 3: " + chain3);  // 150
print("Chain 4: " + chain4);  // 0

// Test 10: Null propagation with boundary conditions
print("Test 10: Null propagation with boundary conditions");
TestData zero = new TestData(0, "zero");
TestData negative = new TestData(-5, "negative");

function processBoundary(TestData? data): string {
    if (data != null) {
        if (data.getValue() > 0) { return "positive"; }
        if (data.getValue() < 0) { return "negative"; }
        return "zero";
    }
    return "null";
}

print("Valid boundary: " + processBoundary(validData));   // positive
print("Zero boundary: " + processBoundary(zero));         // zero
print("Negative boundary: " + processBoundary(negative)); // negative
print("Null boundary: " + processBoundary(nullData));     // null

print("Null propagation through complex expressions test completed successfully");