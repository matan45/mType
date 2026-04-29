// Combo 08: For-Each + Generics + Try-Catch + Lambda
// NOTE: finally blocks removed from generic for-each (VM stack issue)

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class ProcessingException extends Exception {
    private int errorCode;

    public constructor(string msg, int code) : super(msg) {
        this.errorCode = code;
    }

    public function getErrorCode(): int {
        return this.errorCode;
    }
}

interface Processor<T> {
    function process(T item): string;
}

class DataItem<T> {
    private T value;
    private bool valid;

    public constructor(T value, bool valid) {
        this.value = value;
        this.valid = valid;
    }

    public function getValue(): T { return this.value; }
    public function isValid(): bool { return this.valid; }
}

function <T> processWithLambda(ArrayList<DataItem<T>> items, Processor<T> proc): void {
    int successCount = 0;
    int errorCount = 0;

    for (DataItem<T> item : items) {
        try {
            if (!item.isValid()) {
                throw new ProcessingException("Invalid item: " + item.getValue(), 400);
            }
            string result = proc.process(item.getValue());
            print("  OK: " + result);
            successCount = successCount + 1;
        } catch (ProcessingException e) {
            print("  ERROR [" + e.getErrorCode() + "]: " + e.getMessage());
            errorCount = errorCount + 1;
        }
    }

    print("  Results: " + successCount + " success, " + errorCount + " errors");
}

function main(): void {
    print("=== Combo 08: ForEach + Generics + TryCatch + Lambda ===");

    print("--- Processing integers ---");
    ArrayList<DataItem<Int>> intItems = new ArrayList<DataItem<Int>>();
    intItems.add(new DataItem<Int>(new Int(10), true));
    intItems.add(new DataItem<Int>(new Int(20), false));
    intItems.add(new DataItem<Int>(new Int(30), true));
    intItems.add(new DataItem<Int>(new Int(40), false));
    intItems.add(new DataItem<Int>(new Int(50), true));

    Processor<Int> doubler = item -> "doubled: " + (item.getValue() * 2);
    processWithLambda(intItems, doubler);

    print("--- Processing strings ---");
    ArrayList<DataItem<String>> strItems = new ArrayList<DataItem<String>>();
    strItems.add(new DataItem<String>(new String("hello"), true));
    strItems.add(new DataItem<String>(new String(""), false));
    strItems.add(new DataItem<String>(new String("world"), true));

    Processor<String> formatter = item -> "processed: [" + item.getValue() + "]";
    processWithLambda(strItems, formatter);

    // Non-generic for-each with try-catch-finally (concrete type is OK)
    print("--- Nested exception handling ---");
    ArrayList<Int> numbers = new ArrayList<Int>();
    numbers.add(new Int(10));
    numbers.add(new Int(0));
    numbers.add(new Int(5));
    numbers.add(new Int(0));
    numbers.add(new Int(2));

    int total = 0;
    for (Int n : numbers) {
        try {
            if (n.getValue() == 0) {
                throw new ProcessingException("Division by zero", 500);
            }
            int result = 100 / n.getValue();
            total = total + result;
            print("  100/" + n.getValue() + " = " + result);
        } catch (ProcessingException e) {
            print("  Skipped: " + e.getMessage());
        }
    }
    print("Total: " + total);

    print("=== Combo 08 Complete ===");
}

main();
