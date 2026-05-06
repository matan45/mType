// MYT-281: regression guard — the `.length` intrinsic still works after the
// Object widening (it's a dedicated opcode, not a method dispatch).
print("Testing array length still works");

int[] a = new int[5];
print("Direct length: " + a.length);

Object o = a;
int[] back = (int[])o;
print("After round-trip length: " + back.length);

string[][] m = new string[3][4];
print("2D outer length: " + m.length);

print("Done");
