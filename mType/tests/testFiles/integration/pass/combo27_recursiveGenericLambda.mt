// Combo 27: Recursive generic function with lambda predicate
// Tests: generic recursion that walks a list using a lambda as termination check.
// Returns an int index instead of T? because mType's Phase 1 generics don't
// substitute T? cleanly when assigning to a concrete-typed variable.

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function <T> findFirstAt(ArrayList<T> list, Predicate<T> pred, int index): int {
    if (index >= list.size()) {
        return -1;
    }
    T candidate = (T)list.get(index);
    if (pred.test(candidate)) {
        return index;
    }
    return findFirstAt<T>(list, pred, index + 1);
}

function <T> findFirstIndex(ArrayList<T> list, Predicate<T> pred): int {
    return findFirstAt<T>(list, pred, 0);
}

function main(): void {
    print("=== Combo 27: Recursive Generic Lambda ===");

    print("--- Find in Int list ---");
    ArrayList<Int> nums = new ArrayList<Int>();
    nums.add(new Int(2));
    nums.add(new Int(4));
    nums.add(new Int(7));
    nums.add(new Int(9));
    nums.add(new Int(12));

    Predicate<Int> firstOdd = n -> n.getValue() % 2 != 0;
    int oddIdx = findFirstIndex<Int>(nums, firstOdd);
    if (oddIdx >= 0) {
        Int oddHit = (Int)nums.get(oddIdx);
        print("First odd: " + oddHit.getValue());
    } else {
        print("First odd: none");
    }

    Predicate<Int> bigger = n -> n.getValue() > 10;
    int bigIdx = findFirstIndex<Int>(nums, bigger);
    if (bigIdx >= 0) {
        Int bigHit = (Int)nums.get(bigIdx);
        print("First > 10: " + bigHit.getValue());
    } else {
        print("First > 10: none");
    }

    Predicate<Int> impossible = n -> n.getValue() > 999;
    int noIdx = findFirstIndex<Int>(nums, impossible);
    if (noIdx >= 0) {
        Int noHit = (Int)nums.get(noIdx);
        print("Found unexpected: " + noHit.getValue());
    } else {
        print("First > 999: none");
    }

    print("--- Find in String list ---");
    ArrayList<String> words = new ArrayList<String>();
    words.add(new String("hi"));
    words.add(new String("yo"));
    words.add(new String("hello"));
    words.add(new String("hey"));

    Predicate<String> longWord = s -> s.length() >= 5;
    int wIdx = findFirstIndex<String>(words, longWord);
    if (wIdx >= 0) {
        String wHit = (String)words.get(wIdx);
        print("First long word: " + wHit.getValue());
    } else {
        print("First long word: none");
    }

    print("=== Combo 27 Complete ===");
}

main();
