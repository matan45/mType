// Test: Exception handling in constructor
// Expected: Pass - demonstrates constructor with exception handling
import * from "../../lib/exceptions/Exception.mt";

class Account {
    private string owner;
    private int balance;

    public constructor(string owner, int initialBalance) {
        if (initialBalance < 0) {
            print("Error: Cannot create account with negative balance");
            throw new Exception("InvalidBalanceException");
        }
        this.owner = owner;
        this.balance = initialBalance;
        print("Account created for " + owner + " with balance: " + initialBalance);
    }

    public function getBalance(): int {
        return this.balance;
    }

    public function getOwner(): string {
        return this.owner;
    }
}

// Test valid account creation
print("Creating valid account:");
try {
    Account acc1 = new Account("Alice", 1000);
    print("Success - Owner: " + acc1.getOwner() + ", Balance: " + acc1.getBalance());
} catch (Exception e) {
    print("Caught exception: " + e.getMessage());
}

// Test invalid account creation
print("\nCreating invalid account:");
try {
    Account acc2 = new Account("Bob", -500);
    print("This should not print");
} catch (Exception e) {
    print("Caught exception: " + e.getMessage());
}

print("\nProgram continues after exception");
