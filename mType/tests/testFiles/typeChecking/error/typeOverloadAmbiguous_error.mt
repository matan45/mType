// ERROR: Ambiguous method call with overloading

class Processor {
    // Method taking int and string
    public static function process(int num, string msg): string {
        return "Int-String: " + num + ", " + msg;
    }

    // Method taking string and int (reverse order)
    public static function process(string msg, int num): string {
        return "String-Int: " + msg + ", " + num;
    }

    // Another potential ambiguity with nullable types
    public static function handle(int? value): string {
        return "Nullable int: " + value;
    }

    public static function handle(int value): string {
        return "Non-nullable int: " + value;
    }
}

function main(): void {
    // This is fine - clear parameter order
    print(Processor.process(42, "test"));

    // ERROR: Ambiguous when both nullable and non-nullable match
    // Calling with non-null int could match either handle method
    int val = 10;
    print(Processor.handle(val));  // Ambiguous if both are viable

    // Another ambiguous scenario with implicit conversions
    print(Processor.process("hello", 99));
}

main();
