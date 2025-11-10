// Test: Attempting to override final methods
// Expected: Error - final methods cannot be overridden in subclasses

class PaymentProcessor {
    protected float balance;

    public constructor(float initialBalance) {
        this.balance = initialBalance;
    }

    // Final method - cannot be overridden
    public final function validateAmount(float amount): bool {
        return amount > 0.0 && amount <= this.balance;
    }

    // Regular method - can be overridden
    public function processPayment(float amount): bool {
        if (this.validateAmount(amount)) {
            this.balance = this.balance - amount;
            print("Payment processed: " + amount);
            return true;
        }
        print("Payment failed: invalid amount");
        return false;
    }

    // Final method - cannot be overridden
    public final function getBalance(): float {
        return this.balance;
    }
}

class SecurePaymentProcessor extends PaymentProcessor {
    private int securityLevel;

    public constructor(float initialBalance, int securityLevel) : super(initialBalance) {
        this.securityLevel = securityLevel;
    }

    // ERROR: Cannot override final method validateAmount
    public function validateAmount(float amount): bool {
        print("Attempting to override final method");
        return amount > 0.0 && amount <= this.balance && this.securityLevel > 5;
    }

    // OK: processPayment is not final, can be overridden
    public function processPayment(float amount): bool {
        print("Secure processing with level " + this.securityLevel);
        return super.processPayment(amount);
    }

    // ERROR: Cannot override final method getBalance
    public function getBalance(): float {
        print("Attempting to override final method getBalance");
        return this.balance * 1.0;
    }
}

// This should fail during class registration
SecurePaymentProcessor processor = new SecurePaymentProcessor(1000.0, 10);
processor.processPayment(50.0);
