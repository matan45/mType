// MYT-281: an explicit `(T[])obj` cast with a wrong element type errors at
// runtime. Mirrors Java's ClassCastException and pins that the Object
// widening does NOT make casts indiscriminate.
print("Testing downcast wrong element");

int[] ints = new int[3];
ints[0] = 1;

Object o = ints;

// Cast to the wrong array element type: must error.
string[] bad = (string[])o;

print("This should not be reached");
