// Test: Advanced HashMap stream operations
// Tests filtering and complex operations on HashMap streams

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/functional/Function.mt";

function main(): void {
    HashMap<String, Int> scores = new HashMap<String, Int>();
    scores.put(new String("Alice"), new Int(95));
    scores.put(new String("Bob"), new Int(72));
    scores.put(new String("Charlie"), new Int(88));
    scores.put(new String("David"), new Int(65));
    scores.put(new String("Eve"), new Int(91));

    print("=== Filter values > 80 ===");
    Stream<Int> highScores = scores.streamValues()
        .filter(score -> score > 80);
    Int[] highScoreArray = highScores.toArray();
    print("High scores count: " + highScoreArray.length);

    print("=== Map keys ===");
    Stream<String> keyStream = scores.stream()
        .map(key -> new String(key + "_PROCESSED"));
    String[] processedKeys = keyStream.toArray();
    print("Processed keys count: " + processedKeys.length);

    print("=== Count entries with scores >= 70 ===");
    Stream<MapEntry<String, Int>> entryStream = scores.streamEntries();
    int passingCount = 0;
    entryStream.forEach(entry -> {
        if (entry.getValue() >= 70) {
            passingCount = passingCount + 1;
        }
        return;
    });
    print("Students with passing scores: " + passingCount);

    print("=== Filter entries ===");
    Stream<MapEntry<String, Int>> excellentStream = scores.streamEntries()
        .filter(entry -> entry.getValue() >= 90);
    MapEntry<String, Int>[] excellent = excellentStream.toArray();
    print("Excellent students count: " + excellent.length);

    print("HashMap advanced stream test passed!");
}

main();
