// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises boxed Int/Float/Bool/String value-class methods together.

import * from "../lib/primitives/Int.mt";
import * from "../lib/primitives/Float.mt";
import * from "../lib/primitives/Bool.mt";
import * from "../lib/primitives/String.mt";

int N = 500000;
Int one = new Int(1);
Float fone = new Float(1.25);
Bool truth = new Bool(true);
String prefix = new String("item");

Int acc = new Int(0);
Float facc = new Float(0.0);
int boolHits = 0;
int stringTotal = 0;

for (int i = 0; i < N; i = i + 1) {
    Int next = new Int(i % 17);
    acc = acc.add(next).subtract(one);
    facc = facc.add(fone);

    Bool flag = new Bool(i % 2 == 0);
    if (flag.xor(truth).not().getValue()) {
        boolHits = boolHits + 1;
    }

    String text = prefix.concat(new String("-" + (i % 13)));
    stringTotal = stringTotal + text.length();
}

print("boxed_primitive_dispatch_hot acc=" + acc.getValue() + " facc=" + facc.getValue() + " bools=" + boolHits + " strlen=" + stringTotal);
