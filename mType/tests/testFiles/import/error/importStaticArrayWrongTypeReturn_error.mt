// MYT-278 (negative): returning a string[] static method from an int[]-typed
// function must still error after the fix. Mirrors the cross-file ticket repro
// because same-file static metadata is not in program.functions at type-check
// time (separate registration-order issue).
import { A } from "../pass/staticArrayReturn/A.mt";

function bad(): int[] {
    return A::names();
}

int[] result = bad();
print(result[0]);
