// Test: alias in selective source import
import {Int as Number} from "../../../lib/primitives/Int.mt";

Number n = new Number(99);
Number m = new Number(1);
Number sum = n + m;
print(sum);
print("Multi alias test passed");
