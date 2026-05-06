// Edge: extreme whitespace around identifiers and commas inside the braces.
// The lexer/parser must handle arbitrary whitespace without changing the
// semantics of the import.
import {   Calculator   ,    divide   } from "selective_import_utils.mt";

function main(): void {
    Calculator c = new Calculator();
    print(c.add(2, 3));
    float q = divide(10.0, 2.0);
    print("q=" + q);
}

main();
