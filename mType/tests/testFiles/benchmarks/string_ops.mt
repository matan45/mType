// TARGET: ~2s on dev machine. Adjust counts if first run lands outside 1-5s.
// Exercises string concatenation and string equality through the StringPool.

int concatN = 20000;
string s = "";
for (int i = 0; i < concatN; i = i + 1) {
    s = s + "x";
}

int eqN = 1000000;
int matches = 0;
string target = "hello";
for (int i = 0; i < eqN; i = i + 1) {
    if (target == "hello") {
        matches = matches + 1;
    }
}

print("string_ops concat_iters=" + concatN + " matches=" + matches);
