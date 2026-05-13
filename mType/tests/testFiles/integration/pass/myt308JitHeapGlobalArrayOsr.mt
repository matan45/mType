// MYT-308 follow-up: the original ticket repro
// (myt308JitHeapDeadStoreSentinel.mt) only exercises the function-level JIT
// gate, because the dead-store `i = 0; i = -1` pattern is recognized at both
// the function and OSR gates. A real-world demo (rts_step13.mt) uses the
// *same* heap-over-global-arrays shape *without* the sentinel:
//
//     while (i > 0) {                       // heapPush
//         if (aFScore[aHeap[parent]] <= ...) { break; }
//         ...
//         i = parent;
//     }
//     while (true) {                        // heapPop
//         ...
//         if (best != i) { ...; i = best; } else { break; }
//     }
//
// shouldCompileFunction() rejects these via hasGlobalArrayLoopShape(), but
// shouldCompileLoopForOSR() did not — so after ~500 back-edges OSR would
// tier-up the inner while into native code and crash. This test runs enough
// rounds to clear the OSR threshold many times over and pins the no-sentinel
// global-array heap shape so the OSR gate cannot silently regress.

int N = 512;
int[] aFScore = new int[N];
int[] aHeap   = new int[N];
int aHeapCount = 0;

function heapPush(int tile): void {
    aHeap[aHeapCount] = tile;
    aHeapCount = aHeapCount + 1;
    int i = aHeapCount - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (aFScore[aHeap[parent]] <= aFScore[aHeap[i]]) {
            break;
        }
        int tmp = aHeap[parent];
        aHeap[parent] = aHeap[i];
        aHeap[i] = tmp;
        i = parent;
    }
}

function heapPop(): int {
    int top = aHeap[0];
    aHeapCount = aHeapCount - 1;
    if (aHeapCount > 0) {
        aHeap[0] = aHeap[aHeapCount];
        int i = 0;
        while (true) {
            int left  = 2 * i + 1;
            int right = 2 * i + 2;
            int best  = i;
            if (left  < aHeapCount && aFScore[aHeap[left]]  < aFScore[aHeap[best]]) { best = left; }
            if (right < aHeapCount && aFScore[aHeap[right]] < aFScore[aHeap[best]]) { best = right; }
            if (best != i) {
                int tmp = aHeap[i];
                aHeap[i] = aHeap[best];
                aHeap[best] = tmp;
                i = best;
            } else {
                break;
            }
        }
    }
    return top;
}

print("starting");
int totalFails = 0;
for (int round = 0; round < 8; round = round + 1) {
    aHeapCount = 0;
    for (int i = 0; i < N; i = i + 1) {
        aFScore[i] = (i * 73856093 + round * 19349663);
        if (aFScore[i] < 0) { aFScore[i] = -aFScore[i]; }
        aFScore[i] = aFScore[i] % 100000;
        heapPush(i);
    }
    int lastPop = -1;
    int ascendingFails = 0;
    while (aHeapCount > 0) {
        int t = heapPop();
        if (lastPop >= 0 && aFScore[t] < aFScore[lastPop]) {
            ascendingFails = ascendingFails + 1;
        }
        lastPop = t;
    }
    totalFails = totalFails + ascendingFails;
}
print("totalFails=" + totalFails);
print("done");
