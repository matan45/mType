// MYT-281: store-time invariance — the Java unsoundness `Object[] a = new
// String[1]; a[0] = 5;` MUST be rejected. The array runtime tracks its
// element type and rejects a primitive being stored into a String[] slot,
// regardless of how the array is currently aliased.
print("Testing array store invariance");

string[] strs = new string[2];

// Aliasing a string[] as Object via the MYT-281 widening is allowed.
Object o = strs;

// Cast back through a wrong-element type is the surface that must error.
// Even if the cast itself were tolerant, the element store catches the
// primitive-into-string-slot violation.
int[] wrong = (int[])o;
wrong[0] = 5;

print("This should not be reached");
