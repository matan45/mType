class BankAccount {
    public static int totalAccounts = 0;
    public static float interestRate = 0.03;

    public int accountNumber;
    public float balance;

    public constructor(float initialBalance) {
        totalAccounts = totalAccounts + 1;
        accountNumber = totalAccounts;
        balance = initialBalance;
    }

    public function deposit(float amount): void {
        balance = balance + amount;
    }

    public function withdraw(float amount): void {
        if (amount <= balance) {
            balance = balance - amount;
        }
    }

    public function applyInterest(): void {
        balance = balance * (1.0 + interestRate);
    }

    public function getBalance(): float {
        return balance;
    }

    public static function setInterestRate(float rate): void {
        interestRate = rate;
    }

    public static function getTotalAccounts(): int {
        return totalAccounts;
    }
}

BankAccount acc1 = new BankAccount(1000.0);
BankAccount acc2 = new BankAccount(2000.0);

print(BankAccount::getTotalAccounts()); // 2

acc1.deposit(500.0);
print(acc1.getBalance()); // 1500.0

BankAccount::setInterestRate(0.05);
acc1.applyInterest();
print(acc1.getBalance()); // 1575.0

acc2.withdraw(500.0);
print(acc2.getBalance()); // 1500.0