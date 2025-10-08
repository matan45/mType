// Test multiple await expressions in a single function

class Number {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

print("=== Multiple Awaits Test ===");

function async getFirst(): Promise<Number> {
    Number n = new Number(10);
    return n;
}

function async getSecond(): Promise<Number> {
    Number n = new Number(20);
    return n;
}

function async getThird(): Promise<Number> {
    Number n = new Number(30);
    return n;
}

// Function with multiple awaits
function async sumAll(): Promise<Number> {
    // Each await unwraps Promise<Number> to Number
    Number n1 = await getFirst();
    Number n2 = await getSecond();
    Number n3 = await getThird();

    int sum = n1.getValue() + n2.getValue() + n3.getValue();
    Number result = new Number(sum);
    return result;
}

// Main function to run test
function async main(): Promise<Number> {
    Number total = await sumAll();
    print("Total: " + total.getValue());
    return total;
}

main();
