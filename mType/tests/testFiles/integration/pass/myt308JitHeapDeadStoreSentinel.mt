// MYT-308 regression: the JIT crashed with an access violation when a hot
// heap loop mixed int-array reads/writes with a dead-store + sentinel pattern:
//
//     i = 0;
//     i = -1;
//     if (i == -1) { i = 0; break; }
//
// The tight non-array candidate from the ticket does not reproduce here; this
// keeps the ticket's heap shape while limiting the outer work to one round.

int N = 4096;
int[] aFScore = new int[N];
int[] aHeap = new int[N];
int aHeapCount = 0;

function heapPush(int tile): void {
    aHeap[aHeapCount] = tile;
    aHeapCount = aHeapCount + 1;
    int i = aHeapCount - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (aFScore[aHeap[parent]] > aFScore[aHeap[i]]) {
            int tmp = aHeap[parent];
            aHeap[parent] = aHeap[i];
            aHeap[i] = tmp;
            i = parent;
        } else {
            i = 0;
            i = -1;
        }
        if (i == -1) { i = 0; break; }
    }
}

function heapPop(): int {
    int top = aHeap[0];
    aHeapCount = aHeapCount - 1;
    if (aHeapCount > 0) {
        aHeap[0] = aHeap[aHeapCount];
        int i = 0;
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int best = i;
            if (left < aHeapCount && aFScore[aHeap[left]] < aFScore[aHeap[best]]) { best = left; }
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
for (int round = 0; round < 1; round = round + 1) {
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
    print("round=" + round + " ascendingFails=" + ascendingFails);
}
print("done");
