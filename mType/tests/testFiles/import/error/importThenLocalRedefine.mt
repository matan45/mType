// Edge: import a class, then declare a local class with the same name in the
// same file. The compiler must reject this rather than silently shadowing
// one with the other.
import { Calculator } from "selective_import_utils.mt";

class Calculator {
    public function add(int a, int b): int {
        return a + b;
    }
}

function main(): void {
    Calculator c = new Calculator();
    print(c.add(1, 2));
}

main();
