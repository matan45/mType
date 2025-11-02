// Test: Import interface with generic constraints
// @Script

import "modules/ComparableInterface.mt";

class Score : Comparable<Score> {
    private value: Int;

    constructor(v: Int) {
        this.value = v;
    }

    getValue() : Int {
        return this.value;
    }

    compareTo(other: Score) : Int {
        if (this.value < other.getValue()) {
            return -1;
        } else if (this.value > other.getValue()) {
            return 1;
        } else {
            return 0;
        }
    }

    equals(other: Score) : Bool {
        return this.value == other.getValue();
    }
}

class Sorter<T> {
    sort(items: T[], comparator: Comparable<T>) : T[] {
        // Simple bubble sort for demonstration
        let n = items.length();
        let i: Int = 0;
        while (i < n - 1) {
            let j: Int = 0;
            while (j < n - i - 1) {
                let comp = items[j] as Comparable<T>;
                if (comp.compareTo(items[j + 1]) > 0) {
                    // Swap
                    let temp = items[j];
                    items[j] = items[j + 1];
                    items[j + 1] = temp;
                }
                j = j + 1;
            }
            i = i + 1;
        }
        return items;
    }
}

main() : Void {
    let s1 = new Score(50);
    let s2 = new Score(30);
    let s3 = new Score(40);

    assert(s1.compareTo(s2) > 0, "50 > 30");
    assert(s2.compareTo(s1) < 0, "30 < 50");
    assert(s1.compareTo(s1) == 0, "50 == 50");
    assert(s1.equals(new Score(50)), "Scores should be equal");

    print("Comparable interface tests passed");
}
