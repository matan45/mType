// Test generic self-reference pattern (Comparable)
// @Script

import * from "../../lib/collections/ArrayList.mt";

interface Comparable<T extends Comparable<T>> {
    function compareTo(T other): int;
    function isGreaterThan(T other): bool;
}

class Score implements Comparable<Score> {
    private int points;

    public constructor(int points) {
        this.points = points;
    }

    public function compareTo(Score other): int {
        if (this.points < other.points) {
            return -1;
        }
        if (this.points > other.points) {
            return 1;
        }
        return 0;
    }

    public function isGreaterThan(Score other): bool {
        return this.compareTo(other) > 0;
    }

    public function getPoints(): int {
        return this.points;
    }
}

class Ranking<T extends Comparable<T>> {
    private ArrayList<T> items;

    public constructor() {
        this.items = new ArrayList<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function findHighest(): T? {
        if (this.items.size() == 0) {
            return null;
        }

        T highest = this.items.get(0);
        for (int i = 1; i < this.items.size(); i++) {
            T current = this.items.get(i);
            if (current.isGreaterThan(highest)) {
                highest = current;
            }
        }
        return highest;
    }
}

Ranking<Score> ranking = new Ranking<Score>();
ranking.add(new Score(100));
ranking.add(new Score(250));
ranking.add(new Score(175));

Score highest = ranking.findHighest();
if (highest != null) {
    print("Highest score: " + highest.getPoints());
}

Score s1 = new Score(100);
Score s2 = new Score(200);

if (s2.isGreaterThan(s1)) {
    print("s2 is greater than s1");
}
