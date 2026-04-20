// MYT-199: LOAD_LOCAL_BOOL / STORE_LOCAL_BOOL correctness. The `flag`
// local alternates true/false across iterations; the opcodes at its
// LOAD / STORE sites promote to the BOOL variants after the first hit.

function countTrues(int n): int {
    int count = 0;
    bool flag = true;
    for (int i = 0; i < n; i = i + 1) {
        if (flag) {
            count = count + 1;
            flag = false;
        } else {
            flag = true;
        }
    }
    return count;
}

print(countTrues(1000));
