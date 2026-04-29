// Error: downcast between unrelated types must fail at runtime (ClassCastException)
import * from "../../lib/primitives/Int.mt";

class Foo {}

Object o = new Foo();
// Foo and Int are unrelated reference types - this cast must fail
Int i = (Int)o;
print(i.getValue());
