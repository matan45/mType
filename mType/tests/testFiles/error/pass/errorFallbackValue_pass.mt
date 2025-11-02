// Test: Fallback value when operation fails
// Expected: Should return default value on exception
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Custom exception
class DataNotFoundException extends Exception {
    public constructor(string msg) {
        super(msg);
    }
}

// Generic fallback wrapper
function withFallback<T>(function(): T operation, T fallbackValue): T {
    try {
        return operation();
    } catch (Exception e) {
        print("Operation failed, using fallback: " + e.getMessage());
        return fallbackValue;
    }
}

// Test data access function
function getUserName(int userId): string {
    print("Fetching user " + userId);

    if (userId == 1) {
        return "Alice";
    } else if (userId == 2) {
        return "Bob";
    } else {
        throw new DataNotFoundException("User " + userId + " not found");
    }
}

function getUserAge(int userId): int {
    print("Fetching age for user " + userId);

    if (userId == 1) {
        return 25;
    } else if (userId == 2) {
        return 30;
    } else {
        throw new DataNotFoundException("Age for user " + userId + " not found");
    }
}

// Test 1: Successful operation (no fallback needed)
print("=== Test 1: Success - no fallback ===");
string name1 = withFallback(() => getUserName(1), "Guest");
print("Result: " + name1);

// Test 2: Failed operation (use fallback)
print("\n=== Test 2: Failure - use fallback ===");
string name2 = withFallback(() => getUserName(999), "Guest");
print("Result: " + name2);

// Test 3: Integer fallback
print("\n=== Test 3: Integer fallback ===");
int age1 = withFallback(() => getUserAge(1), 0);
print("Age: " + age1);

int age2 = withFallback(() => getUserAge(999), 0);
print("Age: " + age2);

// Test 4: Chained fallback operations
print("\n=== Test 4: Chained fallback operations ===");

function getPrimarySource(): string {
    print("Trying primary source");
    throw new Exception("Primary source unavailable");
}

function getSecondarySource(): string {
    print("Trying secondary source");
    return "Data from secondary";
}

// First fallback tries secondary, second fallback uses default
string data = withFallback(
    () => withFallback(() => getPrimarySource(), getSecondarySource()),
    "Default data"
);
print("Final result: " + data);

// Test 5: Fallback with computation
print("\n=== Test 5: Computed fallback ===");

function riskyCalculation(int value): int {
    if (value < 0) {
        throw new Exception("Negative values not allowed");
    }
    return value * value;
}

int result1 = withFallback(() => riskyCalculation(5), -1);
print("Calculation 1: " + result1);

int result2 = withFallback(() => riskyCalculation(-5), -1);
print("Calculation 2: " + result2);

// Test 6: Fallback chain with multiple levels
print("\n=== Test 6: Multi-level fallback ===");

class Config {
    public string value;

    public constructor(string val) {
        value = val;
    }

    public function getValue(): string {
        return value;
    }
}

function loadFromFile(): Config {
    print("Attempting to load from file");
    throw new Exception("File not found");
}

function loadFromEnvironment(): Config {
    print("Attempting to load from environment");
    throw new Exception("Environment variable not set");
}

function loadDefault(): Config {
    print("Loading default config");
    return new Config("default-value");
}

Config config = withFallback(
    () => withFallback(
        () => loadFromFile(),
        loadFromEnvironment()
    ),
    loadDefault()
);

print("Config value: " + config.getValue());

// Test 7: Null-safe fallback pattern
print("\n=== Test 7: Null-safe access ===");

class User {
    public string name;
    public int? age;

    public constructor(string n) {
        name = n;
        age = null;
    }

    public function getAge(): int {
        if (age == null) {
            throw new Exception("Age not set");
        }
        return age;
    }
}

User user1 = new User("Charlie");
int userAge = withFallback(() => user1.getAge(), 18);
print("User age (with fallback): " + userAge);

// Test 8: Exception type-specific fallbacks
print("\n=== Test 8: Multiple fallback strategies ===");

function withTypedFallback(function(): string operation): string {
    try {
        return operation();
    } catch (DataNotFoundException e) {
        print("Data not found: " + e.getMessage());
        return "NOT_FOUND";
    } catch (Exception e) {
        print("Generic error: " + e.getMessage());
        return "ERROR";
    }
}

string r1 = withTypedFallback(() => {
    throw new DataNotFoundException("Record missing");
});
print("Result 1: " + r1);

string r2 = withTypedFallback(() => {
    throw new Exception("System error");
});
print("Result 2: " + r2);

string r3 = withTypedFallback(() => "SUCCESS");
print("Result 3: " + r3);

print("\nFallback value test completed!");
