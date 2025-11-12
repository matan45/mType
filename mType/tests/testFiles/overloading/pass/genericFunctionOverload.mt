import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/Float.mt";

// Generic function overloading with different type parameters
function <T> process(T value): void {
    print("Generic T: " + value);
}

function <T, K> process(T first, K second): void {
    print("Generic T, K: " + first + ", " + second);
}

function process(int value): void {
    print("Specific int: " + value);
}


function main(): void {
    process(42);              // Should call specific int version (most specific)
    process<String>("Hello");         // Should call generic T version
    process<Int,String>(10, "World");     // Should call generic T, K version
    process<Bool,Float>(true, 3.14);      // Should call generic T, K version
}
main();