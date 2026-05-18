// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises switch dispatch where the discriminant is a string. Companion to
// switch_dispatch_hot.mt (switch on int) — measures the string-comparison
// cost path through the switch executor.

function classify(string s): int {
    int v = 0;
    switch (s) {
        case "alpha":   v = 1;  break;
        case "beta":    v = 2;  break;
        case "gamma":   v = 3;  break;
        case "delta":   v = 4;  break;
        case "epsilon": v = 5;  break;
        case "zeta":    v = 6;  break;
        case "eta":     v = 7;  break;
        case "theta":   v = 8;  break;
        case "iota":    v = 9;  break;
        case "kappa":   v = 10; break;
        default:        v = -1; break;
    }
    return v;
}

string[] keys = ["alpha", "beta", "gamma", "delta", "epsilon",
                 "zeta", "eta", "theta", "iota", "kappa"];

int N = 1000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + classify(keys[i % 10]);
}
print("switch_on_string_hot acc=" + acc);
