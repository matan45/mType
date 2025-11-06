// Test: Import interface with generic constraints
// @Script

import "modules/ComparableInterface.mt";

class Score implements Comparable<Score> {
    private Int value;

    constructor(v: Int) {
        this.value = v;
    }

    public function getValue() : Int {
        return this.value;
    }

    public function compareTo(other: Score) : Int {
        if (this.value < other.getValue()) {
            return -1;
        } else if (this.value > other.getValue()) {
            return 1;
        } else {
            return 0;
        }
    }

    public function equals(other: Score) : Bool {
        return this.value == other.getValue();
    }
}

class Sorter<T> {
    public function sort(items: T[], comparator: Comparable<T>) : T[] {
        // Simple bubble sort for demonstration
        Int n = items.length();
        Int i = 0;
        while (i < n - 1) {
            Int j = 0;
            while (j < n - i - 1) {
                Comparable<T> comp = items[j] as Comparable<T>;
                if (comp.compareTo(items[j + 1]) > 0) {
                    // Swap
                    T temp = items[j];
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

function main() : void {
    Score s1 = new Score(50);
    Score s2 = new Score(30);
    Score s3 = new Score(40);

    assert(s1.compareTo(s2) > 0, "50 > 30");
    assert(s2.compareTo(s1) < 0, "30 < 50");
    assert(s1.compareTo(s1) == 0, "50 == 50");
    assert(s1.equals(new Score(50)), "Scores should be equal");

    print("Comparable interface tests passed");
}
