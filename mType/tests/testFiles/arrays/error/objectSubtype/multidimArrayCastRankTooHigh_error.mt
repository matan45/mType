// MYT-281: a rank-3 multi-dim array cast to a rank-2 array type errors at
// runtime. Pre-fix, the cast handler silently passed any multi-dim source
// through to ANY array target — a soundness hole that hid loss of dimension
// information. The cast handler now reconstructs the runtime type as
// `int[][][]` and compares against `int[][]`, which fails the match and
// throws ClassCastException.
print("Testing rank-too-high multi-dim cast");

int[][][] a = new int[2][2][2];
a[0][0][0] = 1;

Object o = a;
int[][] m = (int[][])o;

print("This should not be reached");
