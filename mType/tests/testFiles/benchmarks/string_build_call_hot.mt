// TARGET: hot function containing STRING_BUILD.
// This makes string interpolation show up in function-level JIT diagnostics,
// not just top-level loop wall-clock time.

function makeLabel(int i, int total): string {
    return $"k-{i % 32}-t-{total % 17}";
}

int N = 100000;
int hits = 0;
int total = 0;

for (int i = 0; i < N; i = i + 1) {
    string s = makeLabel(i, total);
    if (s != "") {
        hits = hits + 1;
    }
    total = total + hits;
}

print("string_build_call_hot hits=" + hits + " total=" + total);
