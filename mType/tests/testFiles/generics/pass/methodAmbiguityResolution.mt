import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method ambiguity resolution
class Processor {
    public static function <T> process(T value): String {
        return new String("Generic: " + value);
    }

    public static function processSpecific(String value): String {
        return new String("Specific: " + value);
    }
}

function main(): void {
    // Specific method should be chosen over generic
    String result1 = Processor.processSpecific(new String("test"));
    print(result1);

    // Generic method with explicit type
    String result2 = Processor.process(new Int(42));
    print(result2);
}

main();
