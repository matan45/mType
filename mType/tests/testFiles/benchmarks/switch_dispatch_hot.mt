// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises switch/case/default dispatch in a tight loop.

function select(int x): int {
    int out = 0;
    switch (x) {
        case 0: out = 3; break;
        case 1: out = 5; break;
        case 2: out = 7; break;
        case 3: out = 11; break;
        case 4: out = 13; break;
        case 5: out = 17; break;
        case 6: out = 19; break;
        case 7: out = 23; break;
        case 8: out = 29; break;
        case 9: out = 31; break;
        case 10: out = 37; break;
        case 11: out = 41; break;
        case 12: out = 43; break;
        case 13: out = 47; break;
        case 14: out = 53; break;
        case 15: out = 59; break;
        default: out = 1; break;
    }
    return out;
}

int N = 3000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + select(i % 17);
}

print("switch_dispatch_hot acc=" + acc);
