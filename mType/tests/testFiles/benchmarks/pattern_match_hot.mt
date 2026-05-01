// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises match value-pattern dispatch in a tight loop.

function classify(int code): int {
    int out = 0;
    match (code) {
        case 0 -> out = 3;
        case 1 -> out = 5;
        case 2 -> out = 7;
        case 3 -> out = 11;
        case 4 -> out = 13;
        case 5 -> out = 17;
        case 6 -> out = 19;
        default -> out = 1;
    }
    return out;
}

int N = 3000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + classify(i % 9);
}

print("pattern_match_hot acc=" + acc);
