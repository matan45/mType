// Integration Test 18: Strategy Pattern with Async Operations
// Tests: Interfaces + Async methods + Polymorphism

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";

// Strategy interface for payment processing
interface PaymentStrategy {
    function async processPayment(float amount): Promise<String>;
    function async validatePayment(float amount): Promise<Bool>;
    function getName(): string;
}

// Concrete strategies
class CreditCardStrategy implements PaymentStrategy {
    private string cardNumber;

    constructor(string card) {
        this.cardNumber = card;
    }

    public function async processPayment(float amount): Promise<String> {
        print("Processing credit card payment: $" + amount);

        // Simulate validation
        bool valid = await this.validatePayment(amount);

        if (valid) {
            return new String("Credit card payment of $" + amount + " successful");
        } else {
            return new String("Credit card payment failed");
        }
    }

    public function async validatePayment(float amount): Promise<Bool> {
        print("Validating credit card...");
        if (amount > 0.0 && amount < 10000.0) {
            return new Bool(true);
        }
        return new Bool(false);
    }

    public function getName(): string {
        return "CreditCard";
    }
}

class PayPalStrategy implements PaymentStrategy {
    private string email;

    constructor(string e) {
        this.email = e;
    }

    public function async processPayment(float amount): Promise<String> {
        print("Processing PayPal payment: $" + amount);

        bool valid = await this.validatePayment(amount);

        if (valid) {
            return new String("PayPal payment of $" + amount + " successful");
        } else {
            return new String("PayPal payment failed");
        }
    }

    public function async validatePayment(float amount): Promise<Bool> {
        print("Validating PayPal account...");
        if (amount > 0.0) {
            return new Bool(true);
        }
        return new Bool(false);
    }

    public function getName(): string {
        return "PayPal";
    }
}

class CryptoStrategy implements PaymentStrategy {
    private string walletAddress;

    constructor(string wallet) {
        this.walletAddress = wallet;
    }

    public function async processPayment(float amount): Promise<String> {
        print("Processing crypto payment: $" + amount);

        bool valid = await this.validatePayment(amount);

        if (valid) {
            return new String("Crypto payment of $" + amount + " successful");
        } else {
            return new String("Crypto payment failed");
        }
    }

    public function async validatePayment(float amount): Promise<Bool> {
        print("Validating crypto wallet...");
        if (amount > 0.0 && amount < 50000.0) {
            return new Bool(true);
        }
        return new Bool(false);
    }

    public function getName(): string {
        return "Crypto";
    }
}

// Context class using strategy
class PaymentProcessor {
    private PaymentStrategy strategy;

    constructor(PaymentStrategy strat) {
        this.strategy = strat;
    }

    public function setStrategy(PaymentStrategy strat): void {
        this.strategy = strat;
        print("Payment strategy changed to: " + strat.getName());
    }

    public function async executePayment(float amount): Promise<String> {
        print("--- Executing payment via " + this.strategy.getName() + " ---");
        String result = await this.strategy.processPayment(amount);
        return result;
    }
}

// Sorting strategy interface
interface SortStrategy {
    function async sort(int[] data): Promise<String>;
    function getName(): string;
}

class QuickSortStrategy implements SortStrategy {
    public function async sort(int[] data): Promise<String> {
        print("QuickSort: Sorting " + data.length + " elements");
        // Simulate sorting
        return new String("QuickSort completed");
    }

    public function getName(): string {
        return "QuickSort";
    }
}

class MergeSortStrategy implements SortStrategy {
    public function async sort(int[] data): Promise<String> {
        print("MergeSort: Sorting " + data.length + " elements");
        // Simulate sorting
        return new String("MergeSort completed");
    }

    public function getName(): string {
        return "MergeSort";
    }
}

class Sorter {
    private SortStrategy strategy;

    constructor(SortStrategy strat) {
        this.strategy = strat;
    }

    public function setStrategy(SortStrategy strat): void {
        this.strategy = strat;
    }

    public function async performSort(int[] data): Promise<String> {
        return await this.strategy.sort(data);
    }
}

// Main async function
function async main(): Promise<void> {
    print("=== Test 18: Strategy Pattern with Async Operations ===");

    // Test 1: Credit card payment
    print("--- Test 1: Credit card ---");
    CreditCardStrategy ccStrategy = new CreditCardStrategy("1234-5678-9012-3456");
    PaymentProcessor processor = new PaymentProcessor(ccStrategy);

    String result1 = await processor.executePayment(150.0);
    print("Result: " + result1.getValue());

    // Test 2: Switch to PayPal
    print("--- Test 2: Switch to PayPal ---");
    PayPalStrategy ppStrategy = new PayPalStrategy("user@example.com");
    processor.setStrategy(ppStrategy);

    String result2 = await processor.executePayment(75.5);
    print("Result: " + result2.getValue());

    // Test 3: Switch to Crypto
    print("--- Test 3: Switch to Crypto ---");
    CryptoStrategy cryptoStrategy = new CryptoStrategy("0x123abc...");
    processor.setStrategy(cryptoStrategy);

    String result3 = await processor.executePayment(500.0);
    print("Result: " + result3.getValue());

    // Test 4: Invalid amount
    print("--- Test 4: Invalid payment ---");
    String result4 = await processor.executePayment(75000.0);
    print("Result: " + result4.getValue());

    // Test 5: Sorting strategies
    print("--- Test 5: Sorting strategies ---");
    int[] data = [5, 2, 8, 1, 9, 3];

    QuickSortStrategy quickSort = new QuickSortStrategy();
    Sorter sorter = new Sorter(quickSort);

    String sortResult1 = await sorter.performSort(data);
    print("Result: " + sortResult1.getValue());

    print("Switching to MergeSort:");
    MergeSortStrategy mergeSort = new MergeSortStrategy();
    sorter.setStrategy(mergeSort);

    String sortResult2 = await sorter.performSort(data);
    print("Result: " + sortResult2.getValue());

    // Test 6: Multiple payments in sequence
    print("--- Test 6: Sequential payments ---");
    PaymentProcessor proc2 = new PaymentProcessor(new CreditCardStrategy("9999-8888-7777-6666"));

    String seq1 = await proc2.executePayment(100.0);
    print(seq1.getValue());

    proc2.setStrategy(new PayPalStrategy("another@example.com"));
    String seq2 = await proc2.executePayment(200.0);
    print(seq2.getValue());

    print("=== Test 18 Complete ===");
}

main();
