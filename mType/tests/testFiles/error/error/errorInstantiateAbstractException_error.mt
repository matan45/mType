// An abstract class — even one extending Exception — cannot be
// instantiated directly. `throw new AbstractEx(...)` must be rejected
// with a compile error mentioning "abstract".
import * from "../../lib/exceptions/Exception.mt";

abstract class AbstractEx extends Exception {
    public constructor(string m) : super(m) {}
    abstract function category(): string;
}

function main(): void {
    throw new AbstractEx("nope");
}
main();
