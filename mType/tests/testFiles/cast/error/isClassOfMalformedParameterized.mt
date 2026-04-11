// Error: malformed parameterized type on the RHS of isClassOf is rejected
// at parse / compile time. `Box<>` has no type argument — the existing
// TypeParser surfaces this as a parse error.

import * from "../../lib/primitives/Box.mt";

Box<Int> box = new Box<Int>(new Int(1));
print(box isClassOf Box<>); // Malformed: empty type argument list
