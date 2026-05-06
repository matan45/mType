// MYT-281: a generic function with `T extends Object` accepts arrays. The
// type parameter binds to the array's reference shape; the array value
// passes through and round-trips identity. MYT-282 fixed the argument
// inferencer to bind T precisely to "int[]" (instead of the coarse
// "array") and the return-context conflict resolver to accept the array
// subtype where "Object" was expected.
print("Testing array in generic Object parameter");

function <T extends Object> wrap(T x): T {
    return x;
}

int[] a = new int[2];
a[0] = 7;
a[1] = 8;

Object wrapped = wrap(a);
int[] back = (int[])wrapped;

print("back[0] = " + back[0]);
print("back[1] = " + back[1]);
print("length = " + back.length);

print("Done");
