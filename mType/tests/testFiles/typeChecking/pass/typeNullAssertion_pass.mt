// Test null assertion operator type narrowing
// Validates that asserting non-null narrows the type correctly

class Account {
    string username;
    int balance;

    constructor(string u, int b) {
        username = u;
        balance = b;
    }

    public function getUsername(): string {
        return username;
    }

    public function getBalance(): int {
        return balance;
    }

    public function deposit(int amount): void {
        balance = balance + amount;
    }
}

class Bank {
    Account account;

    constructor() {
        account = null;
    }

    public function openAccount(string username, int initialBalance): void {
        account = new Account(username, initialBalance);
    }

    public function getAccount(): Account {
        return account;
    }

    public function assertAccountExists(): bool {
        return account != null;
    }

    public function performTransaction(int amount): void {
        // Assert account is not null before using
        if (account != null) {
            account.deposit(amount);
            print("Transaction successful. New balance: " + account.getBalance());
        } else {
            print("Error: No account found");
        }
    }
}

function main(): void {
    print("Testing null assertion type narrowing");

    Bank bank = new Bank();

    // Try transaction without account (should fail safely)
    bank.performTransaction(100);

    // Open account and try again
    bank.openAccount("Alice", 500);

    // Assert account exists
    if (bank.assertAccountExists()) {
        print("Account exists");
        Account acc = bank.getAccount();
        // After assertion, we know acc is not null
        print("Username: " + acc.getUsername());
        print("Balance: " + acc.getBalance());
    }

    // Perform transaction with existing account
    bank.performTransaction(200);

    // Verify balance after transaction
    Account finalAccount = bank.getAccount();
    if (finalAccount != null) {
        print("Final balance: " + finalAccount.getBalance());
    }

    print("Null assertion type narrowing test completed");
}

main();
