// Combo 27: Recursive generic function with lambda predicate
// Tests: generic recursion that walks a list using a lambda as termination check

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function <T> findFirstAt(ArrayList<T> list, Predicate<T> pred, int index): T? {
    if (index >= list.size()) {
        return null;
    }
    T candidate = list.get(index);
    if (pred.test(candidate)) {
        return candidate;
    }
    return findFirstAt(list, pred, index + 1);
}

function <T> findFirst(ArrayList<T> list, Predicate<T> pred): T? {
    return findFirstAt(list, pred, 0);
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
    Int? oddHit = findFirst(nums, firstOdd);
    if (oddHit != null) {
        print("First odd: " + oddHit.getValue());
    } else {
        print("First odd: none");
    }

    Predicate<Int> bigger = n -> n.getValue() > 10;
    Int? bigHit = findFirst(nums, bigger);
    if (bigHit != null) {
        print("First > 10: " + bigHit.getValue());
    } else {
        print("First > 10: none");
    }

    Predicate<Int> impossible = n -> n.getValue() > 999;
    Int? noHit = findFirst(nums, impossible);
    if (noHit != null) {
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
    String? wHit = findFirst(words, longWord);
    if (wHit != null) {
        print("First long word: " + wHit.getValue());
    } else {
        print("First long word: none");
    }

    print("=== Combo 27 Complete ===");
}

main();
