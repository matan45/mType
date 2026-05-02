// MYT-263: OSR boxed-mode Bool.getValue must unbox Bool value-object field 0.
// Before the fix, the first interpreted iterations counted correctly, then the
// OSR-compiled loop read every boxed Bool value as false and left hits at 250.

import * from "../../../lib/primitives/Bool.mt";

int N = 2000;
Bool truth = new Bool(true);
int hits = 0;

for (int i = 0; i < N; i = i + 1) {
    Bool flag = new Bool(i % 2 == 0);
    if (flag.xor(truth).not().getValue()) {
        hits = hits + 1;
    }
}

print("inline_osr_bool_value_chain hits=" + hits);
