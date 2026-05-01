// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises string interpolation lowering and pooled string comparisons.

int N = 200000;
int hits = 0;
int total = 0;

for (int i = 0; i < N; i = i + 1) {
    string s = $"item-{i % 100}-total-{total % 17}";
    if (s != "") {
        hits = hits + 1;
    }
    total = total + hits;
}

print("string_interpolation_hot hits=" + hits + " total=" + total);
