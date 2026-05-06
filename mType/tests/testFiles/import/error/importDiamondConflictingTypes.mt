// Edge: two modules export a class with the same name; the same file imports
// both. The symbol table must reject this rather than silently keeping one.
import { Widget } from "./diamondConflict/ModB.mt";
import { Widget } from "./diamondConflict/ModC.mt";

function main(): void {
    Widget w = new Widget();
    print(w.id());
}

main();
