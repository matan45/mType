class BankAccount {
    static int totalAccounts = 0;
    static float interestRate = 0.03;
    
    int accountNumber;
    float balance;
    
    constructor(float initialBalance) {
        totalAccounts = totalAccounts + 1;
        accountNumber = totalAccounts;
        balance = initialBalance;
    }
    
    function deposit(float amount): void {
        balance = balance + amount;
    }
    
    function withdraw(float amount): void {
        if (amount <= balance) {
            balance = balance - amount;
        }
    }
    
    function applyInterest(): void {
        balance = balance * (1.0 + interestRate);
    }
    
    function getBalance(): float {
        return balance;
    }
    
    static function setInterestRate(float rate): void {
        interestRate = rate;
    }
    
    static function getTotalAccounts(): int {
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