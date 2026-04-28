// MYT-221 lock-in: lib/functional/Comparator.mt is now a true SAM interface.
// `Comparator<T>` declares only `compare`, so a lambda binds directly.

import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

Comparator<Int> asc = (a, b) -> a.compareTo(b);
print("compare(3, 5) = " + asc.compare(new Int(3), new Int(5)));
print("compare(5, 3) = " + asc.compare(new Int(5), new Int(3)));
print("compare(4, 4) = " + asc.compare(new Int(4), new Int(4)));
