// Test: Private fields accessible within same class
class BankAccount {
    private float balance;

    public constructor(float initial) {
        balance = initial;  // Accessing private field in same class
    }

    public function deposit(float amount): void {
        balance = balance + amount;  // Accessing private field in same class
    }

    public function getBalance(): float {
        return balance;  // Accessing private field in same class
    }
}

BankAccount account = new BankAccount(100.0);
account.deposit(50.0);
print(account.getBalance());  // Expected: 150
