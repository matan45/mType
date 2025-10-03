// Test: Private fields accessible within same class
class BankAccount {
    private float balance;

    public BankAccount(float initial) {
        balance = initial;  // Accessing private field in same class
    }

    public void deposit(float amount) {
        balance = balance + amount;  // Accessing private field in same class
    }

    public float getBalance() {
        return balance;  // Accessing private field in same class
    }
}

BankAccount account = new BankAccount(100.0);
account.deposit(50.0);
print(account.getBalance());  // Expected: 150
