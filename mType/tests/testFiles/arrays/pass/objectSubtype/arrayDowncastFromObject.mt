// MYT-281: round-trip — array → Object → array preserves identity.
print("Testing array downcast from Object");

int[] a = new int[3];
a[0] = 100;
a[1] = 200;
a[2] = 300;

Object o = a;
int[] back = (int[])o;

print("Length: " + back.length);
print("Element 0: " + back[0]);
print("Element 1: " + back[1]);
print("Element 2: " + back[2]);

// Mutating through `back` is visible through `a` (same array, no copy).
back[1] = 999;
print("After mutation through cast, a[1] = " + a[1]);

print("Done");
