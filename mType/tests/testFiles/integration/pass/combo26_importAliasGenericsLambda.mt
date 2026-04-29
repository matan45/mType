// Combo 26: Import alias + Generics + Lambda
// Tests: aliased generic class import combined with a lambda-typed parameter

import {ArrayList as Vec} from "../../lib/collections/ArrayList.mt";
import {Predicate as Filter} from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function <T> countMatching(Vec<T> items, Filter<T> pred): int {
    int hits = 0;
    for (int i = 0; i < items.size(); i++) {
        if (pred.test(items.get(i))) {
            hits = hits + 1;
        }
    }
    return hits;
}

function main(): void {
    print("=== Combo 26: Import Alias + Generics + Lambda ===");

    print("--- Aliased Vec<Int> ---");
    Vec<Int> nums = new Vec<Int>();
    nums.add(new Int(1));
    nums.add(new Int(2));
    nums.add(new Int(3));
    nums.add(new Int(4));
    nums.add(new Int(5));

    Filter<Int> isEven = n -> n.getValue() % 2 == 0;
    int evens = countMatching(nums, isEven);
    print("Even count: " + evens);

    Filter<Int> gtTwo = n -> n.getValue() > 2;
    print("Greater than 2 count: " + countMatching(nums, gtTwo));

    print("--- Aliased Vec<String> ---");
    Vec<String> words = new Vec<String>();
    words.add(new String("alpha"));
    words.add(new String("beta"));
    words.add(new String("gamma"));
    words.add(new String("hi"));

    Filter<String> longWord = s -> s.length() > 3;
    print("Long words: " + countMatching(words, longWord));

    print("=== Combo 26 Complete ===");
}

main();
