class Account {
    private int balance;

    public constructor(int initial) {
        this.balance = initial;
    }

    public function getBalance(): int {
        return this.balance;
    }

    public function deposit(int amount): void {
        this.balance = this.balance + amount;
    }
}

Account acc = new Account(100);
print(acc.getBalance());
acc.deposit(50);
print(acc.getBalance());
