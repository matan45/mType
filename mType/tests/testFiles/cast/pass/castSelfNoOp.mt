// Test: casting an Int instance to Int is a no-op and preserves value
import * from "../../lib/primitives/Int.mt";

Int x = new Int(5);
Int y = (Int)x;
print(y.getValue());
