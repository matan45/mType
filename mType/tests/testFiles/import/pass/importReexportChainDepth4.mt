// Edge: 4-deep dependency chain via inheritance (ModB extends ModC's class
// extends ModD's class). Re-exports aren't transparent in mType, so we use
// inheritance to thread the dependency. Exercises class resolution and
// vtable construction across three import boundaries.
import { ChainTop } from "./reexportChain/ModB.mt";

function main(): void {
    ChainTop t = new ChainTop();
    print(t.hello());
}

main();
