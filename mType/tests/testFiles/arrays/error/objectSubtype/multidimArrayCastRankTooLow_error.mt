// MYT-281: a rank-2 multi-dim array cast to a rank-3 array type errors at
// runtime. Pre-fix, the cast handler silently passed it through; the new
// reconstruction yields `int[][]` which does not match `int[][][]`, so the
// cast throws ClassCastException.
print("Testing rank-too-low multi-dim cast");

int[][] m = new int[2][2];
m[0][0] = 1;

Object o = m;
int[][][] a = (int[][][])o;

print("This should not be reached");
