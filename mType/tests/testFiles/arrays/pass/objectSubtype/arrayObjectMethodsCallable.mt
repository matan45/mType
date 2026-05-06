// MYT-282: arrays dispatch the four Object methods through the runtime
// intercept (ObjectExecutor::invokeArrayObjectMethod). The test goes
// through the Object-typed slot so the type checker resolves the method
// against lib/Object.mt; at runtime the receiver still has tag ARRAY and
// the intercept fires before any class-instance dispatch.
import * from "../../../lib/Object.mt";

print("Testing array Object methods callable");

int[] a = new int[3];
a[0] = 1;
a[1] = 2;
a[2] = 3;

Object x = a;

print("toString: " + x.toString());
print("getClass: " + x.getClass());
print("hashCode equals self: " + (x.hashCode() == x.hashCode()));

// Reference equality — same bridge → true.
print("equals self: " + x.equals(x));

// Different bridges → false, even with the same element type and length.
int[] b = new int[3];
b[0] = 1;
b[1] = 2;
b[2] = 3;
Object y = b;
print("equals other: " + x.equals(y));

print("Done");
