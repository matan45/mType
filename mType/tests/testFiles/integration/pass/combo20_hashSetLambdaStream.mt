// Combo 20: HashSet<Int> + Stream Pipeline + Lambda
// Tests: HashSet feeds stream, filter, map, count

import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";

function sortAsc(Int[] arr): Int[] {
    int n = arr.length;
    Int[] result = new Int[n];
    for (int i = 0; i < n; i++) {
        result[i] = arr[i];
    }
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (result[j].getValue() > result[j + 1].getValue()) {
                Int tmp = result[j];
                result[j] = result[j + 1];
                result[j + 1] = tmp;
            }
        }
    }
    return result;
}

function main(): void {
    print("=== Combo 20: HashSet + Stream + Lambda ===");

    HashSet<Int> set = new HashSet<Int>();
    for (int i = 1; i <= 10; i++) {
        set.add(new Int(i));
    }

    print("--- Set size ---");
    print("size = " + set.size());

    print("--- Filter > 5 count ---");
    int filteredCount = set.stream()
        .filter(x -> x.getValue() > 5)
        .count();
    print("filtered count = " + filteredCount);

    print("--- Filter > 5, map *2 count ---");
    int mappedCount = set.stream()
        .filter(x -> x.getValue() > 5)
        .map(x -> new Int(x.getValue() * 2))
        .count();
    print("mapped count = " + mappedCount);

    print("--- Mapped values (sorted) ---");
    Stream<Int> mapped = set.stream()
        .filter(x -> x.getValue() > 5)
        .map(x -> new Int(x.getValue() * 2));
    Int[] arr = mapped.toArray();
    Int[] sorted = sortAsc(arr);
    for (int i = 0; i < sorted.length; i++) {
        print("  " + sorted[i].getValue());
    }

    print("=== Combo 20 Complete ===");
}

main();
