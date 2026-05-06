// MYT-281: a String[] aliased as Object[] still rejects a non-string element
// at the store. Belt-and-braces against the Java covariance leak — even if
// declaration-time covariance is allowed, the runtime element-type tag
// pinned at array creation guards every store.
print("Testing array alias invariance");

string[] strs = new string[2];
strs[0] = "first";

// Widen via Object then narrow back to a wrong shape.
Object o = strs;
int[] wrong = (int[])o;

// Even reading length should be fine if the cast errored, but if the cast
// somehow succeeded (which it must NOT in v1), this store catches it.
wrong[0] = 5;

print("This should not be reached");
