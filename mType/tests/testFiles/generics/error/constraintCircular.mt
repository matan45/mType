import * from "../../lib/primitives/Int.mt";

// Circular constraint detection
interface A<T extends B<T>> {
    function getB(): B<T>;
}

interface B<T extends A<T>> {
    function getA(): A<T>;
}

// Error: Circular type constraint dependency
class Implementation<T extends A<T>> {
    T value;
}

function main(): void {
    print("Should not reach here");
}

main();
