// Test: Method chaining and fluent interfaces
// Expected: Pass - demonstrates method chaining pattern

class StringBuilder {
    private string content;

    public constructor() {
        this.content = "";
    }

    public StringBuilder append(string s) {
        this.content = this.content + s;
        print("append('" + s + "') -> '" + this.content + "'");
        return this;
    }

    public StringBuilder appendLine(string s) {
        this.content = this.content + s + "\n";
        print("appendLine('" + s + "') -> length " + this.length());
        return this;
    }

    public StringBuilder clear() {
        this.content = "";
        print("clear() -> empty");
        return this;
    }

    public int length() {
        return 0; // Simplified for testing
    }

    public string toString() {
        return this.content;
    }
}

// Test method chaining
print("Test 1: Chained calls");
StringBuilder sb1 = new StringBuilder();
sb1.append("Hello").append(" ").append("World");
print("Result: '" + sb1.toString() + "'");

print("\nTest 2: Mixed chaining");
StringBuilder sb2 = new StringBuilder();
sb2.append("First").appendLine(" Line").append("Second").appendLine(" Line");
print("Result: '" + sb2.toString() + "'");

print("\nTest 3: Chain with clear");
StringBuilder sb3 = new StringBuilder();
sb3.append("Temp").clear().append("New").append(" Content");
print("Result: '" + sb3.toString() + "'");
