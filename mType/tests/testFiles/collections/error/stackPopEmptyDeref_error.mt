// Error: Stack.pop() on empty returns null; dereferencing throws NPE
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/primitives/Int.mt";

Stack<Int> s = new Stack<Int>();
Int v = s.pop();
int x = v.getValue();
print("should not reach: " + x);
