// Edge: same module imported twice in the same file with disjoint selective
// sets. The resolver must merge symbol sets without double-evaluating the
// module or duplicating its top-level declarations.
import { Calculator } from "selective_import_utils.mt";
import { divide } from "selective_import_utils.mt";

function main(): void {
    Calculator c = new Calculator();
    print(c.add(1, 2));
    float q = divide(8.0, 4.0);
    print("q=" + q);
}

main();
