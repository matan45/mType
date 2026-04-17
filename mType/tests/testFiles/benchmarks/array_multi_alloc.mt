// TARGET: ~2s on dev machine. Adjust N if first run lands outside 1-5s.
// Exercises NEW_ARRAY_MULTI by allocating small multi-dim arrays in a hot
// function. The allocator is its own function returning the array so:
//   (a) the function crosses the hot threshold easily (workaround for
//       MYT-148: OSR fails on top-level loops), and
//   (b) the return value forces the allocation to be live (no DCE).
//
// NOTE: intentionally avoids indexing (m[i][j][k]) and .length on the
// multi-dim array. The JIT's jit_array_get / jit_array_extract_info helpers
// only handle NativeArray today and throw RuntimeException for FlatMultiArray
// receivers, which aborts the process through the SEH-free asmjit frame.
// Out of scope for MYT-146; revisit when the JIT supports multi-dim array
// access on FlatMultiArray / SparseMultiArray.
//
// Baseline coverage for MYT-146.

function allocOne(): int[][][] {
    int[][][] m = new int[4][4][4];
    return m;
}

int N = 100000;
int dummy = 0;
for (int i = 0; i < N; i = i + 1) {
    int[][][] arr = allocOne();
    dummy = dummy + i;
}

print("array_multi_alloc dummy=" + dummy);
