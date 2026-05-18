// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises substring allocation in a hot loop. Companion to string_ops.mt
// (concat / equals) — measures the allocation + StringPool intern path that
// concat doesn't touch.

string base = "abcdefghij";

int N = 500000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    string sub = substring(base, i % 5, 3);
    total = total + strLength(sub);
}
print("substring_hot total=" + total);
