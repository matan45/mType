// MYT-316: deny-list regression. A plain function whose body contains a
// try/catch must not be inlined (HAS_TRY_CATCH in InlineAnalysis) — the
// runtime-dispatched path must execute the exception flow correctly.

import * from "../../../lib/exceptions/Exception.mt";

function safeDiv(int a, int b): int {
    try {
        if (b == 0) {
            throw new Exception("div by zero");
        }
        return a / b;
    } catch (Exception e) {
        return -1;
    }
}

print(safeDiv(10, 2));   // 5
print(safeDiv(10, 0));   // -1
print(safeDiv(100, 7));  // 14
