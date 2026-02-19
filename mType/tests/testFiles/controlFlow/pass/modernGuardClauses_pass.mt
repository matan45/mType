// Modern guard clause patterns for cleaner control flow
// Tests early validation and edge case handling

class ValidationResult {
    public bool valid;
    public string message;

    constructor(bool v, string m) {
        valid = v;
        message = m;
    }
}

// Test 1: Simple guard clause with early return
function processPositiveNumber(int num): string {
    // Guard: check if number is not positive
    if (num <= 0) {
        return "Error: Number must be positive";
    }

    // Main logic only executes for positive numbers
    int result = num * 2;
    return "Processed: " + result;
}

print("Test 1: Simple guard clause");
print(processPositiveNumber(-5));
print(processPositiveNumber(0));
print(processPositiveNumber(10));

// Test 2: Multiple guard clauses
function divide(int numerator, int denominator): string {
    // Guard: check for zero denominator
    if (denominator == 0) {
        return "Error: Cannot divide by zero";
    }

    // Guard: check for negative values
    if (numerator < 0 || denominator < 0) {
        return "Error: Negative values not allowed";
    }

    // Main logic
    int result = numerator / denominator;
    return "Result: " + result;
}

print("Test 2: Multiple guard clauses");
print(divide(10, 0));
print(divide(-10, 5));
print(divide(10, -5));
print(divide(20, 4));

// Test 3: Guard clauses with object validation
class User {
    public string username;
    public int age;

    constructor(string u, int a) {
        username = u;
        age = a;
    }
}

function validateUser(User user): ValidationResult {
    // Guard: null check
    if (user == null) {
        return new ValidationResult(false, "User cannot be null");
    }

    // Guard: empty username
    if (user.username == null) {
        return new ValidationResult(false, "Username cannot be null");
    }

    // Guard: age validation
    if (user.age < 18) {
        return new ValidationResult(false, "User must be 18 or older");
    }

    if (user.age > 120) {
        return new ValidationResult(false, "Invalid age");
    }

    // Main logic - user is valid
    return new ValidationResult(true, "User is valid");
}

print("Test 3: Guard clauses with object validation");
User? nullUser = null;
ValidationResult result1 = validateUser(nullUser);
print(result1.message);

User emptyUser = new User("", 25);
ValidationResult result2 = validateUser(emptyUser);
print(result2.message);

User youngUser = new User("John", 16);
ValidationResult result3 = validateUser(youngUser);
print(result3.message);

User oldUser = new User("Ancient", 150);
ValidationResult result4 = validateUser(oldUser);
print(result4.message);

User validUser = new User("Alice", 30);
ValidationResult result5 = validateUser(validUser);
print(result5.message);

// Test 4: Guard clauses in nested scenarios
function calculateDiscount(int age, int purchaseAmount): string {
    // Guard: validate age
    if (age < 0) {
        return "Error: Invalid age";
    }

    // Guard: validate purchase amount
    if (purchaseAmount <= 0) {
        return "Error: Purchase amount must be positive";
    }

    // Guard: no discount for small purchases
    if (purchaseAmount < 50) {
        return "No discount: Minimum purchase is 50";
    }

    // Calculate discount based on age
    float discount = 0.0;
    if (age < 18) {
        discount = 0.05;
    } else if (age >= 65) {
        discount = 0.15;
    } else {
        discount = 0.1;
    }

    float finalAmount = purchaseAmount * (1.0 - discount);
    return "Final amount: " + finalAmount;
}

print("Test 4: Guard clauses in discount calculation");
print(calculateDiscount(-1, 100));
print(calculateDiscount(25, -50));
print(calculateDiscount(25, 30));
print(calculateDiscount(15, 100));
print(calculateDiscount(70, 100));
print(calculateDiscount(30, 100));

// Test 5: Guard clauses with string validation
function formatName(string firstName, string lastName): string {
    // Guard: null first name
    if (firstName == null) {
        return "Error: First name cannot be null";
    }

    // Guard: null last name
    if (lastName == null) {
        return "Error: Last name cannot be null";
    }

    // Main logic
    return firstName + " " + lastName;
}

print("Test 5: Guard clauses with string validation");
print(formatName("", "Doe"));
print(formatName("John", ""));
print(formatName("Jane", "Smith"));

// Test 6: Guard clauses in loop processing
function sumPositiveNumbers(int[] numbers, int size): int {
    int sum = 0;

    for (int i = 0; i < size; i++) {
        // Guard: skip negative numbers
        if (numbers[i] < 0) {
            continue;
        }

        // Guard: skip zero
        if (numbers[i] == 0) {
            continue;
        }

        // Process only positive numbers
        sum = sum + numbers[i];
    }

    return sum;
}

print("Test 6: Guard clauses in loops");
int[] testArray = new int[5];
testArray[0] = 10;
testArray[1] = -5;
testArray[2] = 0;
testArray[3] = 20;
testArray[4] = -3;
int arraySum = sumPositiveNumbers(testArray, 5);
print("Sum of positive numbers: " + arraySum);

// Test 7: Guard clauses with range validation
function processTemperature(int temp): string {
    // Guard: temperature too low
    if (temp < -50) {
        return "Error: Temperature below valid range";
    }

    // Guard: temperature too high
    if (temp > 150) {
        return "Error: Temperature above valid range";
    }

    // Classify temperature
    if (temp < 0) {
        return "Freezing: " + temp;
    } else if (temp < 20) {
        return "Cold: " + temp;
    } else if (temp < 30) {
        return "Moderate: " + temp;
    } else {
        return "Hot: " + temp;
    }
}

print("Test 7: Guard clauses with range validation");
print(processTemperature(-100));
print(processTemperature(200));
print(processTemperature(-10));
print(processTemperature(15));
print(processTemperature(25));
print(processTemperature(40));

// Test 8: Guard clauses avoiding deep nesting
function processOrder(int quantity, float price, bool inStock): string {
    // Guard: invalid quantity
    if (quantity <= 0) {
        return "Error: Quantity must be positive";
    }

    // Guard: invalid price
    if (price <= 0.0) {
        return "Error: Price must be positive";
    }

    // Guard: out of stock
    if (!inStock) {
        return "Error: Item out of stock";
    }

    // Guard: quantity limit
    if (quantity > 100) {
        return "Error: Quantity exceeds limit";
    }

    // Main logic - process the order
    float total = quantity * price;
    return "Order processed: " + total;
}

print("Test 8: Guard clauses avoiding deep nesting");
print(processOrder(0, 10.0, true));
print(processOrder(5, -10.0, true));
print(processOrder(5, 10.0, false));
print(processOrder(150, 10.0, true));
print(processOrder(5, 10.0, true));

// Test 9: Nested guard clauses with complex conditions
function evaluateScore(int score, bool bonus): string {
    // Guard: invalid score
    if (score < 0 || score > 100) {
        return "Error: Score must be between 0 and 100";
    }

    // Guard: failing score
    if (score < 60) {
        return "Failed: " + score;
    }

    // Apply bonus if applicable
    int finalScore = score;
    if (bonus && score < 95) {
        finalScore = finalScore + 5;
    }

    // Guard: check if bonus pushed score over limit
    if (finalScore > 100) {
        finalScore = 100;
    }

    return "Passed: " + finalScore;
}

print("Test 9: Nested guard clauses with complex conditions");
print(evaluateScore(-10, false));
print(evaluateScore(150, false));
print(evaluateScore(45, false));
print(evaluateScore(75, false));
print(evaluateScore(75, true));
print(evaluateScore(97, true));

// Test 10: Guard clauses in method chain validation
class Account {
    public int balance;

    constructor(int b) {
        balance = b;
    }
}

function withdraw(Account account, int amount): string {
    // Guard: null account
    if (account == null) {
        return "Error: Account is null";
    }

    // Guard: invalid amount
    if (amount <= 0) {
        return "Error: Amount must be positive";
    }

    // Guard: insufficient funds
    if (account.balance < amount) {
        return "Error: Insufficient funds";
    }

    // Process withdrawal
    account.balance = account.balance - amount;
    return "Withdrawal successful: " + account.balance;
}

print("Test 10: Guard clauses in account operations");
Account? nullAccount = null;
print(withdraw(nullAccount, 50));

Account account1 = new Account(100);
print(withdraw(account1, -20));
print(withdraw(account1, 150));
print(withdraw(account1, 50));
print(withdraw(account1, 30));

print("Test passed");
