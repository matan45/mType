// Modern early return patterns for cleaner code structure
// Tests early returns to avoid deep nesting and improve readability

// Test 1: Early return instead of nested if-else
function categorizeNumber(int num): string {
    if (num < 0) {
        return "negative";
    }
    if (num == 0) {
        return "zero";
    }
    if (num < 10) {
        return "single-digit";
    }
    if (num < 100) {
        return "double-digit";
    }
    return "large";
}

print("Test 1: Early return for categorization");
print(categorizeNumber(-5));
print(categorizeNumber(0));
print(categorizeNumber(7));
print(categorizeNumber(42));
print(categorizeNumber(150));

// Test 2: Early return with validation
function calculateFactorial(int n): int {
    // Early return for invalid input
    if (n < 0) {
        return -1;
    }

    // Early return for base cases
    if (n == 0 || n == 1) {
        return 1;
    }

    // Main calculation
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result = result * i;
    }
    return result;
}

print("Test 2: Early return with validation");
print(calculateFactorial(-3));
print(calculateFactorial(0));
print(calculateFactorial(1));
print(calculateFactorial(5));
print(calculateFactorial(6));

// Test 3: Early return in object processing
class Product {
    public string name;
    public float price;
    public int stock;

    constructor(string n, float p, int s) {
        name = n;
        price = p;
        stock = s;
    }
}

function getProductStatus(Product? product): string {
    // Early return for null
    if (product == null) {
        return "invalid";
    }

    // Early return for out of stock
    if (product.stock == 0) {
        return "out-of-stock";
    }

    // Early return for low stock
    if (product.stock < 5) {
        return "low-stock";
    }

    // Early return for expensive items
    if (product.price > 100.0) {
        return "premium";
    }

    // Default case
    return "available";
}

print("Test 3: Early return in object processing");
Product? nullProduct = null;
print(getProductStatus(nullProduct));

Product outOfStock = new Product("Item1", 50.0, 0);
print(getProductStatus(outOfStock));

Product lowStock = new Product("Item2", 50.0, 3);
print(getProductStatus(lowStock));

Product premium = new Product("Item3", 150.0, 10);
print(getProductStatus(premium));

Product available = new Product("Item4", 50.0, 10);
print(getProductStatus(available));

// Test 4: Early return to avoid deep nesting
function processLogin(string username, string password): string {
    // Early return for empty username
    if (username == null) {
        return "Error: Username required";
    }

    // Early return for empty password
    if (password == null) {
        return "Error: Password required";
    }

    // Early return for invalid credentials (simplified)
    if (username != "admin") {
        return "Error: Invalid username";
    }

    if (password != "pass123") {
        return "Error: Invalid password";
    }

    // Success case - only executed if all checks pass
    return "Login successful";
}

print("Test 4: Early return to avoid deep nesting");
print(processLogin("", "pass"));
print(processLogin("user", ""));
print(processLogin("user", "wrong"));
print(processLogin("admin", "wrong"));
print(processLogin("admin", "pass123"));

// Test 5: Early return with multiple conditions
function evaluateGrade(int score): string {
    // Early return for invalid scores
    if (score < 0) {
        return "Invalid: Score below 0";
    }

    if (score > 100) {
        return "Invalid: Score above 100";
    }

    // Early returns for each grade
    if (score >= 90) {
        return "A";
    }

    if (score >= 80) {
        return "B";
    }

    if (score >= 70) {
        return "C";
    }

    if (score >= 60) {
        return "D";
    }

    return "F";
}

print("Test 5: Early return with grade evaluation");
print(evaluateGrade(-10));
print(evaluateGrade(110));
print(evaluateGrade(95));
print(evaluateGrade(85));
print(evaluateGrade(75));
print(evaluateGrade(65));
print(evaluateGrade(55));

// Test 6: Early return in search operations
function findPositionOfValue(int[] array, int size, int target): int {
    // Early return if array would be null (conceptually)
    if (size <= 0) {
        return -1;
    }

    // Search and return immediately when found
    for (int i = 0; i < size; i++) {
        if (array[i] == target) {
            return i;
        }
    }

    // Not found
    return -1;
}

print("Test 6: Early return in search operations");
int[] searchArray = new int[5];
searchArray[0] = 10;
searchArray[1] = 20;
searchArray[2] = 30;
searchArray[3] = 40;
searchArray[4] = 50;

print(findPositionOfValue(searchArray, 5, 30));
print(findPositionOfValue(searchArray, 5, 99));
print(findPositionOfValue(searchArray, 0, 30));

// Test 7: Early return in business logic
class Order {
    public float amount;
    public bool isPaid;
    public bool isShipped;

    constructor(float a, bool p, bool s) {
        amount = a;
        isPaid = p;
        isShipped = s;
    }
}

function canCancelOrder(Order? order): string {
    // Early return for null order
    if (order == null) {
        return "Cannot cancel: Order not found";
    }

    // Early return if already shipped
    if (order.isShipped) {
        return "Cannot cancel: Order already shipped";
    }

    // Early return if not paid (can cancel easily)
    if (!order.isPaid) {
        return "Cancellation allowed: Unpaid order";
    }

    // Paid but not shipped - requires refund
    return "Cancellation requires refund";
}

print("Test 7: Early return in business logic");
Order? nullOrder = null;
print(canCancelOrder(nullOrder));

Order shippedOrder = new Order(100.0, true, true);
print(canCancelOrder(shippedOrder));

Order unpaidOrder = new Order(100.0, false, false);
print(canCancelOrder(unpaidOrder));

Order paidOrder = new Order(100.0, true, false);
print(canCancelOrder(paidOrder));

// Test 8: Early return with state machine pattern
function getNextState(int currentState, int action): int {
    // State 0: Initial
    if (currentState == 0) {
        if (action == 1) {
            return 1;
        }
        return 0;
    }

    // State 1: Processing
    if (currentState == 1) {
        if (action == 2) {
            return 2;
        }
        if (action == 3) {
            return 0;
        }
        return 1;
    }

    // State 2: Complete
    if (currentState == 2) {
        if (action == 4) {
            return 0;
        }
        return 2;
    }

    // Invalid state
    return -1;
}

print("Test 8: Early return with state machine");
print(getNextState(0, 0));
print(getNextState(0, 1));
print(getNextState(1, 2));
print(getNextState(1, 3));
print(getNextState(2, 4));
print(getNextState(2, 0));
print(getNextState(99, 0));

// Test 9: Early return avoiding unnecessary computation
function isPrime(int n): bool {
    // Early return for numbers less than 2
    if (n < 2) {
        return false;
    }

    // Early return for 2 and 3
    if (n == 2 || n == 3) {
        return true;
    }

    // Early return for even numbers
    if (n % 2 == 0) {
        return false;
    }

    // Check odd divisors
    for (int i = 3; i * i <= n; i = i + 2) {
        if (n % i == 0) {
            return false;
        }
    }

    return true;
}

print("Test 9: Early return in prime check");
print(isPrime(1));
print(isPrime(2));
print(isPrime(4));
print(isPrime(7));
print(isPrime(15));
print(isPrime(17));

// Test 10: Early return in recursive helper
function fibonacci(int n): int {
    // Early returns for base cases
    if (n <= 0) {
        return 0;
    }

    if (n == 1) {
        return 1;
    }

    // Recursive case
    return fibonacci(n - 1) + fibonacci(n - 2);
}

print("Test 10: Early return in recursion");
print(fibonacci(0));
print(fibonacci(1));
print(fibonacci(5));
print(fibonacci(7));
print(fibonacci(10));

print("Test passed");
